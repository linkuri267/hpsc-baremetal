#ifndef SMC_H
#define SMC_H

#include "dma.h"

int smc_init(struct dma *dma);
void smc_deinit();

int smc_sram_load(const char *fname);

#endif // SMC_H
