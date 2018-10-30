#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "busid.h"
#include "regops.h"
#include "mmu.h"

// TODO: pull from SMMU_IDR1
#define NUMCB         16
#define NUMPAGENDXB   0x3
#define PAGESIZE      0x1000

// TODO: register map in Qemu model assumes 0x1000 CB size, but minimum allowed by HW is 2 pages (see SMMU_IDR1)
#define NUMPAGES      1 // (2 * (NUMPAGENDXB + 1))

#define SMMU_BASE      0xf92f0000
#define SMMU_CB_SIZE   (NUMPAGES * PAGESIZE) // also size of global address space
#define SMMU_CB_BASE   (SMMU_BASE + 0x10000) // (SMMU_BASE + SMMU_CB_SIZE) // TODO: Qemu model does not agree with HW?
#define SMMU_CBn_BASE(n)  (SMMU_CB_BASE + (n) * SMMU_CB_SIZE)

#define SMMU__SCR0       0x00000
#define SMMU__SMR0       0x00800
#define SMMU__S2CR0      0x00c00
#define SMMU__CBAR0      0x01000
#define SMMU__CBA2R0     0x01800

#define SMMU__CB_SCTLR  0x00000
#define SMMU__CB_TTBR0  0x00020
#define SMMU__CB_TTBR1  0x00028
#define SMMU__CB_TCR    0x00030
#define SMMU__CB_TCR2   0x00010

#define SMMU__SMR0__VALID (1 << 31)
#define SMMU__SCR0__CLIENTPD 0x1
#define SMMU__S2CR0__EXIDVALID 0x400
#define SMMU__CBAR0__TYPE__MASK                      0x30000
#define SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS 0x10000
#define SMMU__CB_SCTLR__M 0x1
#define SMMU__CB_TCR2__PASIZE__40BIT 0b010
#define SMMU__CB_TCR__EAE__SHORT  0x00000000
#define SMMU__CB_TCR__EAE__LONG   0x80000000
#define SMMU__CB_TCR__VMSA8__TG0_4KB   (0b00 << 14)
#define SMMU__CB_TCR__VMSA8__TG0_16KB  (0b10 << 14)
#define SMMU__CB_TCR__VMSA8__TG0_64KB  (0b01 << 14)
#define SMMU__CB_TCR__T0SZ__SHIFT 0
#define SMMU__CB_TCR__TG0_4KB   0x0000
#define SMMU__CB_TCR__TG0_16KB  0x8000
#define SMMU__CB_TCR__TG0_64KB  0x4000
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
#define VMSA8_DESC__NEXT_TABLE_MASK 0x0000ffffffff0000 // Figure D4-15, assuming TA[51:48] == 0

// See 2.6.4 in ARM MMU Architecture Specificaion

#define T0SZ 32  // use Table 0 for 0x0 to 0x0000_0000_FFFF_FFFF
#define TG_KB 64

#define TG_B  (TG_KB << 10)

// The following are from Table 2-11
#if TG_KB == 4
#define TG_BITS 12
#if (25 < T0SZ) && (T0SZ < 33)
#define TABLE_LVL2_BASE_LSB_BIT (37-T0SZ) // "x"
#define TABLE_LVL2_IA_LSB_BIT 30
#define TABLE_LVL2_IA_HSB_BIT (TABLE_LVL2_BASE_LSB_BIT+26) // "y"
#endif // T0SZ

#elif TG_KB == 16
#define TG_BITS 14
#if (28 < T0SZ) && (T0SZ < 38)
#define TABLE_LVL2_BASE_LSB_BIT (42-T0SZ) // "x"
#define TABLE_LVL2_IA_LSB_BIT 25
#define TABLE_LVL2_IA_HSB_BIT (TABLE_LVL2_BASE_LSB_BIT+21) // "y"
#endif // T0SZ

#elif TG_KB == 64
#define TG_BITS 16
#if (22 < T0SZ) && (T0SZ < 34)
#define TABLE_LVL2_BASE_LSB_BIT (38-T0SZ) // "x"
#define TABLE_LVL2_IA_LSB_BIT 29
#define TABLE_LVL2_IA_HSB_BIT (TABLE_LVL2_BASE_LSB_BIT+25) // "y"
#endif // T0SZ
#endif // TG_KB

#ifndef TABLE_LVL2_BASE_LSB_BIT
#error Given T0SZ and TG combination not implemented in this driver
#endif

#define BLOCK_LVL2_SIZE_BITS TABLE_LVL2_IA_LSB_BIT
#define BLOCK_LVL2_SIZE (1 << TABLE_LVL2_IA_LSB_BIT)

#define TABLE_LVL2_IDX_BITS (TABLE_LVL2_IA_HSB_BIT-TABLE_LVL2_IA_LSB_BIT+1)
#define TABLE_LVL2_ALIGN_BITS TABLE_LVL2_BASE_LSB_BIT

#define TABLE_LVL2_ENTRIES (1 << TABLE_LVL2_IDX_BITS) // in number of entries
#define TABLE_LVL2_SIZE (TABLE_LVL2_ENTRIES * 8) // in bytes

#define TABLE_LVL3_IDX_BITS (TABLE_LVL2_IA_LSB_BIT - TG_BITS)  // also Table D4-9
#define TABLE_LVL3_ALIGN_BITS TG_BITS

#define TABLE_LVL3_ENTRIES (1 << TABLE_LVL3_IDX_BITS) // in number of entries
#define TABLE_LVL3_SIZE (TABLE_LVL3_ENTRIES * 8) // in bytes

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (((x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))

#define CONTEXT_PT_SIZE (TABLE_LVL2_SIZE + TABLE_LVL2_ENTRIES * TABLE_LVL3_SIZE)

#define GL_REG(reg)         ((volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + reg))
#define CB_REG32(ctx, reg)  ((volatile uint32_t *)((volatile uint8_t *)SMMU_CBn_BASE(ctx) + reg))
#define CB_REG64(ctx, reg)  ((volatile uint64_t *)((volatile uint8_t *)SMMU_CBn_BASE(ctx) + reg))
#define ST_REG(stream, reg) ((volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + reg) + stream)

#define MAX_CONTEXTS 8
#define MAX_STREAMS  8

static unsigned avail_contexts = 0;
static uint64_t *pt_addr;
static uint64_t *pt_lvl2[MAX_CONTEXTS] = {0}; // could be calculated from pt_addr
static bool streams[MAX_STREAMS]; // indicates whether the stream is allocated or not

int mmu_init(uint32_t ptaddr, unsigned ptspace)
{
    // ASSERT(pt_addr & ~(0xffffffff << TABLE_LVL2_ALIGN_BITS) == 0x0) // check alignment of table base address
    if ((ptaddr & ~(0xffffffff << TABLE_LVL2_ALIGN_BITS)) != 0x0) { // check alignment of table base address
        printf("ERROR: MMU page table base addr not aligned: %p\r\n", pt_addr);
        return -1;
    }

    pt_addr = (uint64_t *)ptaddr;
    printf("ptspace=%x ctx size %x\r\n", ptspace, CONTEXT_PT_SIZE);
    avail_contexts = ptspace / CONTEXT_PT_SIZE;
    avail_contexts = avail_contexts <= MAX_CONTEXTS ? avail_contexts : MAX_CONTEXTS;
    printf("MMU: have mem for %u contexts\r\n", avail_contexts);

    return 0;
}

int mmu_context_create()
{
    unsigned ctx = 0;
    while (ctx < avail_contexts && pt_lvl2[ctx])
        ++ctx;
    if (ctx == avail_contexts) {
        printf("ERROR: MMU: failed to create context: no more\r\n");
        return -1;
    }

    // Initialize two levels of page tables
    uint64_t *pt_lvl2_addr = (uint64_t *)ALIGN((uint32_t)pt_addr + ctx * CONTEXT_PT_SIZE,
                                                TABLE_LVL2_ALIGN_BITS);
    pt_lvl2[ctx] = pt_lvl2_addr;

    uint64_t *pt_lvl3 = (uint64_t *)ALIGN((uint32_t)pt_lvl2_addr + TABLE_LVL2_ENTRIES,
                                           TABLE_LVL3_ALIGN_BITS);
    for (uint32_t i = 0; i < TABLE_LVL2_ENTRIES; ++i) {
        for (uint32_t j = 0; j < TABLE_LVL3_ENTRIES; ++j)
            pt_lvl3[j] = ~VMSA8_DESC__VALID;
        pt_lvl2_addr[i] = ~VMSA8_DESC__VALID;
        pt_lvl3 += TABLE_LVL3_ENTRIES;
    }

    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR),
              (T0SZ << SMMU__CB_TCR__T0SZ__SHIFT) | SMMU__CB_TCR__VMSA8__TG0_64KB);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR2), SMMU__CB_TCR2__PASIZE__40BIT);
    REG_WRITE64(CB_REG64(ctx, SMMU__CB_TTBR0), (uint32_t)pt_lvl2_addr);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_SCTLR), SMMU__CB_SCTLR__M);

    printf("MMU: created ctx %u\r\n", ctx);
    return ctx;
}

int mmu_context_destroy(unsigned ctx)
{
    if (ctx >= avail_contexts) {
        printf("ERROR: MMU invalid context index (%u >= %u)\r\n", ctx, avail_contexts);
        return -1;
    }
    if (!pt_lvl2[ctx]) {
        printf("ERROR: MMU context %u not initialized\r\n", ctx);
        return -1;
    }

    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR), 0);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_TCR2), 0);
    REG_WRITE64(CB_REG64(ctx, SMMU__CB_TTBR0), 0);
    REG_WRITE32(CB_REG32(ctx, SMMU__CB_SCTLR), 0);

    pt_lvl2[ctx] = 0;
    printf("MMU: destroyed ctx %u\r\n", ctx);
    return 0;
}

int mmu_map(unsigned ctx, uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    printf("MMU: map: ctx %u 0x%08x%08x -> 0x%08x%08x size = %x\r\n",
           ctx, (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
           (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

    if (sz != ALIGN(sz, TG_BITS)) {
        printf("ERROR: MMU map: region size (%x) not aligned to TG %x (bits %u)\r\n",
               sz, TG_B, TG_BITS);
        return -1;
    }

    uint64_t *pt_lvl2_addr = pt_lvl2[ctx];

    do { // iterate over pages or blocks
        unsigned index_lvl2 = ((vaddr >> TABLE_LVL2_IA_LSB_BIT) & ~(~0 << TABLE_LVL2_IDX_BITS));

        uint64_t desc_lvl2 = pt_lvl2_addr[index_lvl2];
        if (vaddr == ALIGN(vaddr, BLOCK_LVL2_SIZE_BITS) && sz >= BLOCK_LVL2_SIZE) {

            desc_lvl2 = (paddr & (~0 << VMSA8_DESC__BITS)) |
                VMSA8_DESC__VALID |
                VMSA8_DESC__TYPE__BLOCK |
                VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
                VMSA8_DESC__AP__EL0 |
                VMSA8_DESC__AP__RW |
                VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

            vaddr += BLOCK_LVL2_SIZE;
            paddr += BLOCK_LVL2_SIZE;
            sz -= BLOCK_LVL2_SIZE;
        } else { // not aligned to or sized to block size, so use Level 3 PT

            uint64_t *pt_lvl3 = (uint64_t *)ALIGN((uint32_t)pt_lvl2_addr + TABLE_LVL2_SIZE +
                        index_lvl2 * TABLE_LVL3_SIZE, TABLE_LVL3_ALIGN_BITS);

            // Populate the entry in Lvl 2 PT -- if overwritten by a block mapping
            if (!(desc_lvl2 & VMSA8_DESC__VALID) || 
                ((desc_lvl2 & VMSA8_DESC__VALID) &&
                 (desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__BLOCK)) {
                
                pt_lvl2_addr[index_lvl2] = ((uint64_t)(uint32_t)pt_lvl3 & ~ALIGN_MASK(TABLE_LVL3_ALIGN_BITS)) |
                    VMSA8_DESC__VALID |
                    VMSA8_DESC__TYPE__TABLE;
            }

            unsigned index_lvl3 = ((vaddr >> TG_BITS) & ~(~0 << TABLE_LVL3_IDX_BITS));
            pt_lvl3[index_lvl3] = (paddr & (~0 << VMSA8_DESC__BITS)) |
                VMSA8_DESC__VALID |
                VMSA8_DESC__TYPE__PAGE |
                VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
                VMSA8_DESC__AP__EL0 |
                VMSA8_DESC__AP__RW |
                VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

            vaddr += TG_B;
            paddr += TG_B;
            sz -= TG_B;
        }
   } while (sz > 0);

   for (uint32_t i = 0; i < TABLE_LVL2_ENTRIES; ++i) {
       uint32_t desc_hi = (uint32_t)((pt_lvl2_addr[i] >> 32) & 0xffffffff);
       uint32_t desc_lo = (uint32_t)(pt_lvl2_addr[i]         & 0xffffffff);
       printf("%04u: 0x%p: 0x%08x%08x\r\n", i, &pt_lvl2_addr[i], desc_hi, desc_lo);
   }

   return 0;
}

int mmu_unmap(unsigned ctx, uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    printf("MMU: unmap: ctx %u 0x%08x%08x -> 0x%08x%08x size = %x\r\n",
           ctx, (uint32_t)(vaddr >> 32), (uint32_t)vaddr,
           (uint32_t)(paddr >> 32), (uint32_t)paddr, sz);

    if (sz != ALIGN(sz, TG_BITS)) {
        printf("ERROR: MMU unmap: region size (%x) not aligned to TG %x (bits %u)\r\n",
               sz, TG_B, TG_BITS);
        return -1;
    }

    uint64_t *pt_lvl2_addr = pt_lvl2[ctx];

    do { // iterate over pages or blocks
        unsigned index_lvl2 = ((vaddr >> TABLE_LVL2_IA_LSB_BIT) & ~(~0 << TABLE_LVL2_IDX_BITS));

        uint64_t desc_lvl2 = pt_lvl2_addr[index_lvl2];

        if (!(desc_lvl2 & VMSA8_DESC__VALID))
            continue; // nothing mapped, so nothing to unmap

        if ((desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__BLOCK &&
            (vaddr == ALIGN(vaddr, BLOCK_LVL2_SIZE_BITS) && sz >= BLOCK_LVL2_SIZE)) {

            pt_lvl2_addr[index_lvl2] = ~VMSA8_DESC__VALID;

            vaddr += BLOCK_LVL2_SIZE;
            paddr += BLOCK_LVL2_SIZE;
            sz -= BLOCK_LVL2_SIZE;
        } else if ((desc_lvl2 & VMSA8_DESC__TYPE_MASK) == VMSA8_DESC__TYPE__TABLE) {
            uint64_t *pt_lvl3 = (uint64_t *)((uint32_t)(desc_lvl2 & VMSA8_DESC__NEXT_TABLE_MASK));

            unsigned index_lvl3 = ((vaddr >> TG_BITS) & ~(~0 << TABLE_LVL3_IDX_BITS));
            pt_lvl3[index_lvl3] = ~VMSA8_DESC__VALID;

            vaddr += TG_B;
            paddr += TG_B;
            sz -= TG_B;
        } // otherwise it's a block but size is not aligned (the unmap is not symmetrical with map)
    } while (sz > 0);
    return 0;
}

int mmu_stream_create(unsigned master, unsigned ctx)
{
    int stream = 0;

    while (stream < MAX_STREAMS && streams[stream])
        ++stream;
    if (stream == MAX_STREAMS) {
        printf("ERROR: failed to setup stream: no more match registers\r\n");
        return -1;
    }
    streams[stream] = true;

    printf("stream=%u -> ctx %u\r\n", stream, ctx);

    REG_WRITE32(ST_REG(stream, SMMU__SMR0), SMMU__SMR0__VALID | master);
    REG_WRITE32(ST_REG(stream, SMMU__S2CR0), SMMU__S2CR0__EXIDVALID | ctx);
    REG_WRITE32(ST_REG(stream, SMMU__CBAR0), SMMU__CBAR0__TYPE__STAGE1_WITH_STAGE2_BYPASS);
    REG_WRITE32(ST_REG(stream, SMMU__CBA2R0), SMMU__CBA2R0__VA64);

    printf("MMU: created stream %u -> ctx %u\r\n", stream, ctx);
    return stream;
}

int mmu_stream_destroy(unsigned stream)
{
    if (stream >= MAX_STREAMS) {
        printf("ERROR: MMU: stream destroy: invalid stream %u >= %u\r\n",
               stream, MAX_STREAMS);
        return -1;
    }
    if (!streams[stream]) {
        printf("ERROR: MMU: stream destroy: stream %u does not exist\r\n",
               stream);
        return -1;
    }
    streams[stream] = false;

    REG_WRITE32(ST_REG(stream, SMMU__SMR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__S2CR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__CBAR0), 0);
    REG_WRITE32(ST_REG(stream, SMMU__CBA2R0), 0);

    printf("MMU: destroyed stream %u\r\n", stream);
    return 0;
}

void mmu_enable()
{
    REG_CLEAR32(GL_REG(SMMU__SCR0), SMMU__SCR0__CLIENTPD);
}

void mmu_disable()
{
    REG_SET32(GL_REG(SMMU__SCR0), SMMU__SCR0__CLIENTPD);
}
