#include "smc.h"

#include "board.h"

/* Add params for different memory chips (MRAM, NOR, etc) below */

/* SRAM memory chip modeled in Zebu (at HPPS SMC) */
static const struct smc_mem_chip_cfg lsio_smc_chip_sram_zebu = {
    .width = 32,
    /* from the Reset value of opmode<interface>_<chip> */
    .sync = false,
    .adv = false,
     /* from the Reset value of sram_cycles<interface>_<chip> */
    .t_rc = 12,
    .t_wc = 12,
    .t_ceoe = 3,
    .t_wp = 6,
    .t_pc = 4,
    .t_tr = 1,
};

const struct smc_mem_cfg lsio_smc_mem_cfg = {
    .iface = {
        [SMC_IFACE_SRAM] = {
            /* Memory Interface Configuration Register for direct_cmd */
            .ext_addr_bits = 0xb, /* ?? */
            .cre = true,
            .chips = 4,
            .chip_cfgs = {
                &lsio_smc_chip_sram_zebu, /* TODO: should be an NOR chip */
                &lsio_smc_chip_sram_zebu, /* TODO: should be an NOR chip */
                &lsio_smc_chip_sram_zebu, /* TODO: should be an MRAM chip */
                &lsio_smc_chip_sram_zebu, /* TODO: should be an MRAM chip */
            },
        },
        [SMC_IFACE_NAND] = {
            .chips = 0, /* nothing connected */
        },
    }
};
