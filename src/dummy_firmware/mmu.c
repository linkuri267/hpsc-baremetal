#include <stdint.h>

#include "printf.h"
#include "mmu.h"

// From QEMU device tree / HW spec
#define MASTER_ID_TRCH_CPU  0x2d

#define MASTER_ID_RTPS_CPU0 0x2e
#define MASTER_ID_RTPS_CPU1 0x2f

#define MASTER_ID_HPPS_CPU0 0x80
#define MASTER_ID_HPPS_CPU1 0x8d
#define MASTER_ID_HPPS_CPU2 0x8e
#define MASTER_ID_HPPS_CPU3 0x8f
#define MASTER_ID_HPPS_CPU4 0x90
#define MASTER_ID_HPPS_CPU5 0x9d
#define MASTER_ID_HPPS_CPU6 0x9e
#define MASTER_ID_HPPS_CPU7 0x9f

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
#define VMSA8_DESC__TYPE__BLOCK 0b00
#define VMSA8_DESC__TYPE__TABLE 0b10
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
#define TG_KB 64

#if TG_KB == 4
#define TABLE_BASE_LSB_BIT (37-T0SZ) // "x"
#define TABLE_IA_HSB_BIT (TABLE_BASE_LSB_BIT+26) // "y"
#define TABLE_IA_LSB_BIT 30

#elif TG_KB == 64
#define TABLE_BASE_LSB_BIT (38-T0SZ) // "x"
#define TABLE_IA_HSB_BIT (TABLE_BASE_LSB_BIT+25) // "y"
#define TABLE_IA_LSB_BIT 29
#endif

#define TABLE_IDX_BITS (TABLE_IA_HSB_BIT-TABLE_IA_LSB_BIT+1)
#define TABLE_ALIGN (TABLE_BASE_LSB_BIT + TABLE_IDX_BITS + 3) // TODO: should this be TABLE_BASE_LSB_BIT+1 instead of TABLE_BASE_LSB_BIT?
#define TABLE_SIZE (1 << TABLE_IDX_BITS) // in number of entries

// Page table should be in memory that is on a bus accessible from the MMUs master port ('dma' prop in MMU node in Qemu DT).
// We put it in HPPS DRAM, because that seems to be the only option, judging from high-level Chiplet diagram.
#define RTPS_HPPS_PT_ADDR 0x8e000000

int mmu_init(void *vaddr, uint64_t paddr) { // TODO: figure out a reasonable API

    uint32_t va = (uint32_t)vaddr;

    unsigned index = ((va >> TABLE_IA_LSB_BIT) & ~(~0 << TABLE_IDX_BITS));
    printf("MMU: mapping: 0x%08x -> 0x%08x%08x PT idxbits = %u size = %u idx = %u\r\n", va, (uint32_t)(paddr >> 32), (uint32_t)paddr, TABLE_IDX_BITS, TABLE_SIZE, index);

    uint64_t *rtps_hpps_pt0 = (uint64_t *)RTPS_HPPS_PT_ADDR; // must be aligned to TABLE_ALIGN bits
    for (uint32_t i = 0; i < TABLE_SIZE; ++i) {

        // Identity map for all except the requested one
        uint64_t pa = (i == index) ? paddr : (i << TABLE_IA_LSB_BIT);

        rtps_hpps_pt0[i] = (pa & (~0 << VMSA8_DESC__BITS)) |
                    VMSA8_DESC__VALID |
                    VMSA8_DESC__TYPE__BLOCK |
                    VMSA8_DESC__PXN | VMSA8_DESC__XN | // no execution permission
                    VMSA8_DESC__AP__EL0 |
                    VMSA8_DESC__AP__RW |
                    VMSA8_DESC__AF; // set Access flag (otherwise we need to handle fault)

        uint32_t desc_hi = (uint32_t)((rtps_hpps_pt0[i] >> 32) & 0xffffffff);
        uint32_t desc_lo = (uint32_t)(rtps_hpps_pt0[i]         & 0xffffffff);

        printf("%04u: 0x%p: 0x%08x%08x\r\n", i, &rtps_hpps_pt0[i], desc_hi, desc_lo);
   }

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

    // ASSERT(RTPS_HPPS_PT_ADDR & ~(0xffffffff << TABLE_ALIGN) == 0x0) // check alignment of table base address
    if ((RTPS_HPPS_PT_ADDR & ~(0xffffffff << TABLE_ALIGN)) != 0x0) { // check alignment of table base address
        printf("ERROR: RTPS-HPPS MMU page table base addr not aligned: %p\r\n", RTPS_HPPS_PT_ADDR);
        return -1;
    }

    *addr = RTPS_HPPS_PT_ADDR;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CB0_SCTLR) + ctxidx;
    *addr |= SMMU__CB0_SCTLR__M;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__CBA2R0) + ctxidx;
    *addr |= SMMU__CBA2R0__VA64;

    addr = (volatile uint32_t *)((volatile uint8_t *)SMMU_BASE + SMMU__SCR0);
    *addr &= ~SMMU__SCR0__CLIENTPD; // enable the MMU

    printf("MMU: mapping configured\r\n");

    return 0;
}
