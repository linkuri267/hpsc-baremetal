#include "smc.h"

#include "board.h"

struct smc_mem_cfg trch_smc_mem_cfg = {
    .iface = {
        [0] = { /* interface 0: SRAM */
            /* Config values copied from memory connected to HPPS SMC in Zebu */
            .chips = 4,
            .width = 32,
            .ext_addr_bits = 0xb,
            .sync = true,
            .adv = true,
            .cre = true,
            .t_rc = 10,
            .t_wc = 10,
            .t_ceoe = 1,
            .t_wp = 1,
            .t_pc = 1,
            .t_tr = 1,
        },
        [1] = { /* interface 1: NAND */
            .chips = 0, /* nothing connected */
        },
    }
};

struct smc_mem_iface_cfg trch_smc_boot_init = {
    /* Config values copied from memory connected to HPPS SMC in Zebu */
    /* Memory Interface Configuration Register */
    /* for direct_cmd */
    .chips = 4,
    .ext_addr_bits = 0xb,	/* ?? */
    .cre = true,
    /* from the Reset value of opmode<interface>_<chip> */
    .sync = false,
    .adv = false,
    .width = 32,
     /* from the Reset value of sram_cycles<interface>_<chip> */
    .t_rc = 12,
    .t_wc = 12,
    .t_ceoe = 3,
    .t_wp = 6,
    .t_pc = 4,
    .t_tr = 1,
};

