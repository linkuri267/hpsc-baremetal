#include <stdint.h>

#include "printf.h"
#include "panic.h"
#include "object.h"
#include "regops.h"

#include "smc.h"

#define SMC_REG(b, r) ((b) + (r))

#define SMC__memc_status                0x000
#define SMC__mem_cfg                    0x004
#define SMC__mem_cfg_set                0x008
#define SMC__mem_cfg_clr                0x00c
#define SMC__direct_cmd                 0x010
#define SMC__set_cycles                 0x014
#define SMC__set_opmode                 0x018

#define SMC__cycles_0_0                 0x100
#define SMC__cycles_0_1                 0x120
#define SMC__cycles_0_2                 0x140
#define SMC__cycles_0_3                 0x160

#define SMC__memc_status__raw_int_status0__SHIFT        5
#define SMC__memc_status__raw_int_status1__SHIFT        6

#define SMC__mem_cfg__mem0_type__SHIFT			0
#define SMC__mem_cfg__mem1_type__SHIFT			8
#define SMC__mem_cfg__mem_type__MASK			0x3
#define SMC__mem_cfg__NONE 				0x0
#define SMC__mem_cfg__SRAM__MASK			0x1
#define SMC__mem_cfg__SRAM_NON_MULTIPLEXED		0x1
#define SMC__mem_cfg__NAND				0x2
#define SMC__mem_cfg__SRAM_MULTIPLEXED			0x3

#define SMC__mem_cfg_set__int_enable0__SHIFT            0
#define SMC__mem_cfg_set__int_enable1__SHIFT            1

#define SMC__mem_cfg_clr__int_clear0__SHIFT             3
#define SMC__mem_cfg_clr__int_clear1__SHIFT             4

#define SMC__opmode__set_mw__SHIFT                      0
#define SMC__opmode__rd_sync__SHIFT                     6
#define SMC__opmode__wr_sync__SHIFT                     2
#define SMC__opmode__set_adv__SHIFT                    11

#define SMC__mw__8_BIT                  0b00
#define SMC__mw__16_BIT                 0b01
#define SMC__mw__32_BIT                 0b10

#define SMC__set_opmode__MASK 0x0000ffff
#define SMC__set_cycles__MASK 0x000fffff

#define SMC__set_opmode_addr_match__MASK 0xff000000
#define SMC__set_opmode_addr_mask__MASK 0x00ff0000

#define SMC__cycles__t_rc__SHIFT         0
#define SMC__cycles__t_wc__SHIFT         4
#define SMC__cycles__t_ceoe__SHIFT       8
#define SMC__cycles__t_wp__SHIFT        11
#define SMC__cycles__t_pc__SHIFT        14
#define SMC__cycles__t_tr__SHIFT        17

#define SMC__opmode_addr_match__SHIFT        24
#define SMC__opmode_addr_mask__SHIFT         16

#define SMC__direct_cmd__addr__SHIFT         0
#define SMC__direct_cmd__set_cre__SHIFT           20
#define SMC__direct_cmd__cmd_type__SHIFT          21
#define SMC__direct_cmd__chip_nmbr__SHIFT         23

#define SMC__cmd_type__UpdateRegsAXI	        0b00
#define SMC__cmd_type__ModeReg		        0b10
#define SMC__cmd_type__UpdateRegs	        0b10
#define SMC__cmd_type__ModeRegUpdateRegs        0b11

struct smc {
    uintptr_t base;
};

#define MAX_SMCS 2
static struct smc smcs[MAX_SMCS];

static uint32_t opmode_offset[2][4] = {
    {0x104, 0x124, 0x144, 0x164},
    {0x184, 0x1a4, 0x1c4, 0x1e4}
};

static unsigned to_width_bits(unsigned width)
{
   switch (width) {
       case  8: return SMC__mw__8_BIT;
       case 16: return SMC__mw__16_BIT;
       case 32: return SMC__mw__32_BIT;
    }
    panic("invalid memory bus width");
    return 0;
}

void smc_icfg_init(uintptr_t base, int interface, int chip,
                   struct smc_mem_iface_cfg *icfg)
{
    REG_WRITE32(SMC_REG(base, SMC__set_opmode),
        (icfg->adv << SMC__opmode__set_adv__SHIFT) |
        (icfg->sync << SMC__opmode__rd_sync__SHIFT) |
        (icfg->sync << SMC__opmode__wr_sync__SHIFT) |
        (to_width_bits(icfg->width) << SMC__opmode__set_mw__SHIFT));

    REG_WRITE32(SMC_REG(base, SMC__set_cycles),
        (icfg->t_rc << SMC__cycles__t_rc__SHIFT) |
        (icfg->t_wc << SMC__cycles__t_wc__SHIFT) |
        (icfg->t_ceoe << SMC__cycles__t_ceoe__SHIFT) |
        (icfg->t_wp << SMC__cycles__t_wp__SHIFT) |
        (icfg->t_pc << SMC__cycles__t_pc__SHIFT) |
        (icfg->t_tr << SMC__cycles__t_tr__SHIFT));

    REG_WRITE32(SMC_REG(base, SMC__direct_cmd),
        (icfg->cre << SMC__direct_cmd__set_cre__SHIFT) |
        ((chip + (interface << 2)) << SMC__direct_cmd__chip_nmbr__SHIFT) |
        (SMC__cmd_type__ModeRegUpdateRegs << SMC__direct_cmd__cmd_type__SHIFT) |
        (icfg->ext_addr_bits << SMC__direct_cmd__addr__SHIFT));
}

struct smc *smc_init(uintptr_t base, struct smc_mem_cfg *cfg)
{
    struct smc *s;
    s = OBJECT_ALLOC(smcs);
    s->base = base;

    for (int i = 0; i < SMC_INTERFACES; ++i) {
        struct smc_mem_iface_cfg *icfg = &cfg->iface[i];

        if (icfg->chips == 0)
            continue;

        printf("SMC: init interface %u with %u chips\r\n", i, icfg->chips);

        REG_WRITE32(SMC_REG(base, SMC__set_opmode),
            (icfg->adv << SMC__opmode__set_adv__SHIFT) |
            (icfg->sync << SMC__opmode__rd_sync__SHIFT) |
            (icfg->sync << SMC__opmode__wr_sync__SHIFT) |
            (to_width_bits(icfg->width) << SMC__opmode__set_mw__SHIFT));

        REG_WRITE32(SMC_REG(base, SMC__set_cycles),
            (icfg->t_rc << SMC__cycles__t_rc__SHIFT) |
            (icfg->t_wc << SMC__cycles__t_wc__SHIFT) |
            (icfg->t_ceoe << SMC__cycles__t_ceoe__SHIFT) |
            (icfg->t_wp << SMC__cycles__t_wp__SHIFT) |
            (icfg->t_pc << SMC__cycles__t_pc__SHIFT) |
            (icfg->t_tr << SMC__cycles__t_tr__SHIFT));

          for (int j = 0; j < icfg->chips; ++j) {
            REG_WRITE32(SMC_REG(base, SMC__direct_cmd),
                (icfg->cre << SMC__direct_cmd__set_cre__SHIFT) |
                ((j + (i << 2)) << SMC__direct_cmd__chip_nmbr__SHIFT) |
                (SMC__cmd_type__ModeRegUpdateRegs
                    << SMC__direct_cmd__cmd_type__SHIFT) |
                (icfg->ext_addr_bits << SMC__direct_cmd__addr__SHIFT));
          }
    }
    return s;
}

void smc_deinit(struct smc *s)
{
    ASSERT(s);
    s->base = 0;
    OBJECT_FREE(s);
}

uint32_t *smc_get_base_addr(uintptr_t base, int interface, int rank) {
    uint32_t reg = REG_READ32(SMC_REG(base, opmode_offset[interface][rank]));
    uint32_t match = reg & SMC__set_opmode_addr_match__MASK;
    uint32_t mask = (reg & SMC__set_opmode_addr_mask__MASK) << 8;
    return (uint32_t *) (match & mask);
}

void smc_boot_init(uintptr_t base, int mem_rank, struct smc_mem_iface_cfg *iface_cfg, bool failover) {
    /* sram interface 
     * from mem_rank */

    uint32_t reg = REG_READ32(SMC_REG(base, SMC__mem_cfg));
    uint32_t mem0_type = (reg >> SMC__mem_cfg__mem0_type__SHIFT) & SMC__mem_cfg__mem_type__MASK;
    uint32_t mem1_type = (reg >> SMC__mem_cfg__mem1_type__SHIFT) & SMC__mem_cfg__mem_type__MASK;
    uint32_t interface;

    printf("%s: mem_cfg = 0x%x\n", __func__, reg);
#ifdef HW_EMULATION
    /* find SRAM interface */
    if (mem0_type & SMC__mem_cfg__SRAM__MASK) {
        interface = 0;
    }
    if (mem1_type & SMC__mem_cfg__SRAM__MASK) {
        interface = 1;
        printf("Warning: SRAM  interface is found at interface (1) instead of (0)\n");
    }
#else
    printf("%s: mem0_type(0x%x) and mem1_type(0x%x) are ignored\n", __func__, mem0_type, mem1_type);
    interface = 0;
#endif
    /* Set cycles and memory width */
    smc_icfg_init (base, interface, mem_rank, iface_cfg);
    if (failover) { /* initialize the other chip */
        int mem_rank1 = (mem_rank % 0x1 == 0 ? mem_rank + 1 : mem_rank - 1);
        smc_icfg_init (base, interface, mem_rank1, iface_cfg);
        
    }
}
