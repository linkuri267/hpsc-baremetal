#ifndef SMC_H
#define SMC_H

#include "dma.h"

int smc_init(struct dma *dma);
void smc_deinit();

// addr: set to the load addr found in the image
int smc_sram_load(const char *fname, uint32_t **addr);

#endif // SMC_H
