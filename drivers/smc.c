#include <stdint.h>

#include "printf.h"
#include "panic.h"
#include "object.h"
#include "regops.h"

#include "smc.h"

#define SMC_REG(b, r) ((b) + (r))

#define SMC__memc_status                0x000
#define SMC__mem_cfg_set                0x008
#define SMC__mem_cfg_clr                0x00c
#define SMC__direct_cmd                 0x010
#define SMC__set_cycles                 0x014
#define SMC__set_opmode                 0x018

#define SMC__cycles_0_0                 0x100
#define SMC__cycles_0_1                 0x120
#define SMC__cycles_0_2                 0x140
#define SMC__cycles_0_3                 0x160

#define SMC__opmode_0_0                 0x104
#define SMC__opmode_0_1                 0x124
#define SMC__opmode_0_2                 0x144
#define SMC__opmode_0_3                 0x164

#define SMC__memc_status__raw_int_status0__SHIFT        5
#define SMC__memc_status__raw_int_status1__SHIFT        6

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

#define SMC__cycles__t_rc__SHIFT         0
#define SMC__cycles__t_wc__SHIFT         4
#define SMC__cycles__t_ceoe__SHIFT       8
#define SMC__cycles__t_wp__SHIFT        11
#define SMC__cycles__t_pc__SHIFT        14
#define SMC__cycles__t_tr__SHIFT        17

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
                (j << SMC__direct_cmd__chip_nmbr__SHIFT) |
                (SMC__cmd_type__ModeRegUpdateRegs << SMC__direct_cmd__cmd_type__SHIFT) |
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
