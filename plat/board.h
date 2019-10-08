#ifndef BOARD_H
#define BOARD_H

#include "smc.h"

#define SMC_MEM_RANK_NOR_START      0
#define SMC_MEM_RANK_NOR_COUNT      2
#define SMC_MEM_RANK_MRAM_START     2
#define SMC_MEM_RANK_MRAM_COUNT     2

const struct smc_mem_cfg lsio_smc_mem_cfg;

#endif // BOARD_H
