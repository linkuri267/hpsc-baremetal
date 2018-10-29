#include <stdint.h>

#include "printf.h"
#include "busid.h"
#include "mmu.h"

#define SMMU_BASE      0xf92f0000

#define SMMU__SCR0       0x00000
#define SMMU__SMR0       0x00800
#define SMMU__S2CR0      0x00c00
#define SMMU__CBAR0      0x01000
#define SMMU__CB0_SCTLR  0x10000
#define SMMU__CB0_TTBR0  0x10020
#define SMMU__CB0_TTBR1  0x10028
#define SMMU__CB0_TCR    0x10030
#define SMMU__CB0_TCR2   0x10010
#define SMMU__CBA2R0     0x01800

#define SMMU__SMR0__VALID (1 << 31)
#define SMMU__SCR0__CLIENTPD 0x1
#define SMMU__S2CR0__EXIDVALID 0x400
#define SMMU__CBAR0__TYPE__MASK                      0x30000
#define SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS 0x10000
#define SMMU__CB0_SCTLR__M 0x1
#define SMMU__CB0_TCR2__PASIZE__40BIT 0b010
#define SMMU__CB0_TCR__EAE__SHORT  0x00000000
#define SMMU__CB0_TCR__EAE__LONG   0x80000000
#define SMMU__CB0_TCR__VMSA8__TG0_4KB   (0b00 << 14)
#define SMMU__CB0_TCR__VMSA8__TG0_16KB  (0b10 << 14)
#define SMMU__CB0_TCR__VMSA8__TG0_64KB  (0b01 << 14)
#define SMMU__CB0_TCR__T0SZ__SHIFT 0
#define SMMU__CB0_TCR__TG0_4KB   0x0000
#define SMMU__CB0_TCR__TG0_16KB  0x8000
#define SMMU__CB0_TCR__TG0_64KB  0x4000
#define SMMU__CBA2R0__VA64 0x1

// VMSAv8 Long-descriptor fields
#define VMSA8_DESC__BITS   12 // Lower attributes in stage 1 (D4-2150)
#define VMSA8_DESC__VALID 0b1
#define VMSA8_DESC__TYPE_MASK   0b10
#define VMSA8_DESC__TYPE__BLOCK 0b00
#define VMSA8_DESC__TYPE__TABLE 0b10
#define VMSA8_DESC__TYPE__PAGE  0b10
#define VMSA8_DESC__UNPRIV 0x40
#define VMSA8_DESC__RO     0x80
#define VMSA8_DESC__AF     0x400
#define VMSA8_DESC__AP__EL0    0x40
#define VMSA8_DESC__AP__RO     0x80
#define VMSA8_DESC__AP__RW     0x00
#define VMSA8_DESC__NG     0x800
#define VMSA8_DESC__PXN    (1LL << 53)
#define VMSA8_DESC__XN     (1LL << 54)

// See 2.6.4 in ARM MMU Architecture Specificaion

#define T0SZ 32  // use Table 0 for 0x0 to 0x0000_0000_FFFF_FFFF
#define TG_BITS 16
#define TG_B  (1 << TG_BITS)
#define TG_KB (TG_B >> 10)

#define NEXT_TABLE_ADDR_MASK 0x0000ffffffff0000 // Figure D4-15, assuming TA[51:48] == 0

#if TG_KB == 4

#if (25 < T0SZ) && (T0SZ < 33)
#define TABLE_BASE_LSB_BIT (37-T0SZ) // "x"
#define TABLE_IA_LSB_BIT 30
#define TABLE_IA_HSB_BIT (TABLE_BASE_LSB_BIT+26) // "y"
#else // T0SZ
#error Given T0SZ and TG combination not implemented in this driver
#endif // T0SZ

#define DESC_TABLE_ADDR_OFFSET 12

#elif TG_KB == 64
#if (22 < T0SZ) && (T0SZ < 34)
#define TABLE_BASE_LSB_BIT (38-T0SZ) // "x"
#define TABLE_IA_LSB_BIT 29
#define TABLE_IA_HSB_BIT (TABLE_BASE_LSB_BIT+25) // "y"
#else // T0SZ
#error TABLE_IA_LSB_BIT for given T0SZ not implemented in this driver
#endif // T0SZ

#define DESC_TABLE_ADDR_OFFSET 16
#define DESC_PHYS_ADDR_OFFSET  16 // TODO: same as DESC_TABLE_ADDR_OFFSET by construction?
#define TABLE_LVL3_IA_LSB_BIT  16 // TODO; same as DESC_TABLE_ADDR_OFFSET by construction?

#define TABLE_LVL3_IDX_BITS (TABLE_IA_LSB_BIT - TABLE_LVL3_IA_LSB_BIT)

#define TABLE_LVL3_ALIGN_BITS DESC_TABLE_ADDR_OFFSET
#define TABLE_LVL3_BITS_RESOLVED 13 // Table D4-9
#define TABLE_LVL3_SIZE (1 << TABLE_LVL3_BITS_RESOLVED)


#endif

#define BLOCK_SIZE_BITS TABLE_IA_LSB_BIT
#define BLOCK_SIZE (1 << TABLE_IA_LSB_BIT)

#define TABLE_IDX_BITS (TABLE_IA_HSB_BIT-TABLE_IA_LSB_BIT+1)
#define TABLE_ALIGN_BITS (TABLE_BASE_LSB_BIT + TABLE_IDX_BITS + 3) // TODO: should this be TABLE_BASE_LSB_BIT+1 instead of TABLE_BASE_LSB_BIT?
#define TABLE_SIZE (1 << TABLE_IDX_BITS) // in number of entries

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (((x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))

static uint64_t *pt_lvl2;

int mmu_init(uint32_t pt_addr)
{
    // ASSERT(pt_addr & ~(0xffffffff << TABLE_ALIGN_BITS) == 0x0) // check alignment of table base address
    if ((pt_addr & ~(0xffffffff << TABLE_ALIGN_BITS)) != 0x0) { // check alignment of table base address
        printf("ERROR: RTPS-HPPS MMU page table base addr not aligned: %p\r\n", pt_lvl2);
        return -1;
    }

    // Initialize two levels of page tables
    pt_lvl2 = (uint64_t *)ALIGN((uint32_t)pt_addr, TABLE_ALIGN_BITS); // should be a noop

    uint64_t *pt_lvl3 = (uint64_t *)ALIGN((uint32_t)pt_lvl2 + TABLE_SIZE, TABLE_LVL3_ALIGN_BITS);
    for (uint32_t i = 0; i < TABLE_SIZE; ++i) {
        for (uint32_t j = 0; j < TABLE_LVL3_SIZE; ++j)
            pt_lvl3[j] = ~VMSA8_DESC__VALID;
        pt_lvl2[i] = ~VMSA8_DESC__VALID;
        pt_lvl3 += TABLE_LVL3_SIZE;
    }

    // TODO: create multiple contexted

    unsigned ctxidx = 0;
    volatile uint32_t *addr;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__SMR0) + ctxidx;
    *addr |= SMMU__SMR0__VALID | MASTER_ID_RTPS_CPU0;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__S2CR0) + ctxidx;
    *addr |= SMMU__S2CR0__EXIDVALID;// exidvalid=1, cbndx=?
    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CBAR0) + ctxidx;
    *addr &= ~SMMU__CBAR0__TYPE__MASK;
    *addr |= SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS;
    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CB0_TCR) + ctxidx;
    *addr |= (T0SZ << SMMU__CB0_TCR__T0SZ__SHIFT)
           | SMMU__CB0_TCR__VMSA8__TG0_64KB;
    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CB0_TCR2) + ctxidx;
    *addr |= SMMU__CB0_TCR2__PASIZE__40BIT;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CB0_TTBR0) + ctxidx;
    *addr = (uint32_t)pt_lvl2;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CB0_SCTLR) + ctxidx;
    *addr |= SMMU__CB0_SCTLR__M;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CBA2R0) + ctxidx;
    *addr |= SMMU__CBA2R0__VA64;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__SCR0);
    *addr &= ~SMMU__SCR0__CLIENTPD; // enable the MMU

    printf("MMU: initialized\r\n");

    return 0;
}

int mmu_map(uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    printf("MMU: map: 0x%08x%08x -> 0x%08x%08x size = %x\r\n",
           (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
           (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

    if (sz != ALIGN(sz, TG_BITS)) {
        printf("ERROR: MMU map: region size (%x) not aligned to TG %x (bits %u)\r\n",
               sz, TG_B, TG_BITS);
        return -1;
    }

    do { // iterate over pages or blocks
        unsigned index_lvl2 = ((vaddr >> TABLE_IA_LSB_BIT) & ~(~0 << TABLE_IDX_BITS));

        uint64_t desc_lvl2 = pt_lvl2[index_lvl2];
        if (vaddr == ALIGN(vaddr, BLOCK_SIZE_BITS) && sz >= BLOCK_SIZE) {

            desc_lvl2 = (paddr & (~0 << VMSA8_DESC__BITS)) |
                VMSA8_DESC__VALID |
                VMSA8_DESC__TYPE__BLOCK |
                VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
                VMSA8_DESC__AP__EL0 |
                VMSA8_DESC__AP__RW |
                VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

            vaddr += BLOCK_SIZE;
            paddr += BLOCK_SIZE;
            sz -= BLOCK_SIZE;
        } else { // not aligned to or sized to block size, so use Level 3 PT

            uint64_t *pt_lvl3 = (uint64_t *)ALIGN((uint32_t)pt_lvl2 +
                        index_lvl2 * TABLE_SIZE, TABLE_LVL3_ALIGN_BITS);

            // Populate the entry in Lvl 2 PT -- if overwritten by a block mapping
            if (!(desc_lvl2 & VMSA8_DESC__VALID) || 
                ((desc_lvl2 & VMSA8_DESC__VALID) &&
                 (desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__BLOCK)) {
                
                pt_lvl2[index_lvl2] = ((uint64_t)(uint32_t)pt_lvl3 & ~ALIGN_MASK(TABLE_LVL3_ALIGN_BITS)) |
                    VMSA8_DESC__VALID |
                    VMSA8_DESC__TYPE__TABLE;
            }

            unsigned index_lvl3 = ((vaddr >> TABLE_LVL3_IA_LSB_BIT) & ~(~0 << TABLE_LVL3_IDX_BITS));
            pt_lvl3[index_lvl3] = (paddr & (~0 << VMSA8_DESC__BITS)) |
                VMSA8_DESC__VALID |
                VMSA8_DESC__TYPE__PAGE |
                VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
                VMSA8_DESC__AP__EL0 |
                VMSA8_DESC__AP__RW |
                VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

           for (uint32_t i = 0; i < 4; ++i) {
               uint32_t desc_hi = (uint32_t)((pt_lvl3[i] >> 32) & 0xffffffff);
               uint32_t desc_lo = (uint32_t)(pt_lvl3[i]         & 0xffffffff);
               printf("%04u: 0x%p: 0x%08x%08x\r\n", i, &pt_lvl3[i], desc_hi, desc_lo);
           }


            vaddr += TG_B;
            paddr += TG_B;
            sz -= TG_B;
        }
   } while (sz > 0);

   for (uint32_t i = 0; i < TABLE_SIZE; ++i) {
       uint32_t desc_hi = (uint32_t)((pt_lvl2[i] >> 32) & 0xffffffff);
       uint32_t desc_lo = (uint32_t)(pt_lvl2[i]         & 0xffffffff);
       printf("%04u: 0x%p: 0x%08x%08x\r\n", i, &pt_lvl2[i], desc_hi, desc_lo);
   }


   return 0;
}

int mmu_unmap(uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    printf("MMU: unmap: 0x%08x%08x -> 0x%08x%08x size = %x\r\n",
           (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
           (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

    if (sz != ALIGN(sz, TG_BITS)) {
        printf("ERROR: MMU unmap: region size (%x) not aligned to TG %x (bits %u)\r\n",
               sz, TG_B, TG_BITS);
        return -1;
    }

    do { // iterate over pages or blocks
        unsigned index_lvl2 = ((vaddr >> TABLE_IA_LSB_BIT) & ~(~0 << TABLE_IDX_BITS));

        uint64_t desc_lvl2 = pt_lvl2[index_lvl2];

        if (!(desc_lvl2 & VMSA8_DESC__VALID))
            continue; // nothing mapped, so nothing to unmap

        if ((desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__BLOCK &&
            (vaddr == ALIGN(vaddr, BLOCK_SIZE_BITS) && sz >= BLOCK_SIZE)) {

            pt_lvl2[index_lvl2] = ~VMSA8_DESC__VALID;

            vaddr += BLOCK_SIZE;
            paddr += BLOCK_SIZE;
            sz -= BLOCK_SIZE;
        } else if ((desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE) {
            uint64_t *pt_lvl3 = (uint64_t *)((uint32_t)(desc_lvl2 & NEXT_TABLE_ADDR_MASK));

            unsigned index_lvl3 = ((vaddr >> TABLE_LVL3_IA_LSB_BIT) & ~(~0 << TABLE_LVL3_IDX_BITS));
            pt_lvl3[index_lvl3] = ~VMSA8_DESC__VALID;

            vaddr += TG_B;
            paddr += TG_B;
            sz -= TG_B;
        } // otherwise it's a block but size is not aligned (the unmap is not symmetrical with map)
    } while (sz > 0);
    return 0;
}
