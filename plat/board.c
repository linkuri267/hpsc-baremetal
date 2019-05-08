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
