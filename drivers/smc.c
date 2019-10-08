#include <stdint.h>

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

#define SMC__mem_cfg__mem_type__SHIFT			8
#define SMC__mem_cfg__mem_type__MASK			0x3

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

    /* smc_iface_type -> index */
    unsigned iface_index[SMC_INTERFACES];
};

#define MAX_SMCS 2
static struct smc smcs[MAX_SMCS];

static const uint32_t opmode_offset[2][4] = {
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

static void smc_stage_cfg(uintptr_t base,
                          const struct smc_mem_chip_cfg *chip_cfg)
{
    REG_WRITE32(SMC_REG(base, SMC__set_opmode),
        (chip_cfg->adv << SMC__opmode__set_adv__SHIFT) |
        (chip_cfg->sync << SMC__opmode__rd_sync__SHIFT) |
        (chip_cfg->sync << SMC__opmode__wr_sync__SHIFT) |
        (to_width_bits(chip_cfg->width) << SMC__opmode__set_mw__SHIFT));

    REG_WRITE32(SMC_REG(base, SMC__set_cycles),
        (chip_cfg->t_rc << SMC__cycles__t_rc__SHIFT) |
        (chip_cfg->t_wc << SMC__cycles__t_wc__SHIFT) |
        (chip_cfg->t_ceoe << SMC__cycles__t_ceoe__SHIFT) |
        (chip_cfg->t_wp << SMC__cycles__t_wp__SHIFT) |
        (chip_cfg->t_pc << SMC__cycles__t_pc__SHIFT) |
        (chip_cfg->t_tr << SMC__cycles__t_tr__SHIFT));
}

static void smc_apply_staged_cfg(uintptr_t base,
                                 unsigned interface, unsigned chip,
                                 const struct smc_mem_iface_cfg *icfg)
{
    REG_WRITE32(SMC_REG(base, SMC__direct_cmd),
        (icfg->cre << SMC__direct_cmd__set_cre__SHIFT) |
        ((chip + (interface << 2)) << SMC__direct_cmd__chip_nmbr__SHIFT) |
        (SMC__cmd_type__ModeRegUpdateRegs << SMC__direct_cmd__cmd_type__SHIFT) |
        (icfg->ext_addr_bits << SMC__direct_cmd__addr__SHIFT));
}

static enum smc_mem_type smc_get_mem_type(uintptr_t base, unsigned interface)
{
    uint32_t reg = REG_READ32(SMC_REG(base, SMC__mem_cfg));
    return (reg >> (interface * SMC__mem_cfg__mem_type__SHIFT))
                    & SMC__mem_cfg__mem_type__MASK;
}

struct smc *smc_init(uintptr_t base, const struct smc_mem_cfg *cfg,
                     uint8_t iface_mask, uint8_t chip_mask)
{
    struct smc *s;
    enum smc_mem_type mem_type;
    const struct smc_mem_iface_cfg *icfg;
    const struct smc_mem_chip_cfg *chip_cfg;

    s = OBJECT_ALLOC(smcs);
    s->base = base;

    /* The type of memory at the interface is queried dynamically from SMC */
    for (int iface = 0; iface < SMC_INTERFACES; ++iface) {
        mem_type = smc_get_mem_type(base, iface);
        switch (mem_type) {
            case SMC_MEM_SRAM_NON_MULTIPLEXED:
                if (!(iface_mask & SMC_IFACE_SRAM_MASK))
                    continue;
                s->iface_index[SMC_IFACE_SRAM] = iface;
                icfg = &cfg->iface[SMC_IFACE_SRAM];

                DPRINTF("SMC: init interface %u as SRAM type with %u chips\r\n",
                       iface, icfg->chips);
                for (int chip = 0; chip < icfg->chips; ++chip) {
                    if (!(chip_mask & (1 << chip)))
                        continue;
                    chip_cfg = icfg->chip_cfgs[chip];
                    smc_stage_cfg(base, chip_cfg);
                    smc_apply_staged_cfg(base, iface, chip, icfg);
                }
            case SMC_MEM_NONE:
                break;
            default:
                DPRINTF("SMC: not initializing interface #%u: "
                       "mem type %x is not supported by driver\r\n",
                       iface, mem_type);
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

uint8_t *smc_get_base_addr(struct smc *s, enum smc_iface_type iface_type,
                           unsigned rank)
{
    uint32_t reg = REG_READ32(SMC_REG(s->base,
        opmode_offset[s->iface_index[iface_type]][rank]));
    uint32_t match = reg & SMC__set_opmode_addr_match__MASK;
    uint32_t mask = (reg & SMC__set_opmode_addr_mask__MASK) << 8;
    return (uint8_t *) (match & mask);
}
