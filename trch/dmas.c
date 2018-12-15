#include "printf.h"
#include "dma.h"
#include "nvic.h"
#include "hwinfo.h"

#include "dmas.h"

static struct dma *trch_dma;

static uint8_t trch_dma_mcode[256]; // store in TRCH SRAM

struct dma *trch_dma_init()
{
    trch_dma = dma_create("TRCH", TRCH_DMA_BASE,
            trch_dma_mcode, sizeof(trch_dma_mcode));
    if (!trch_dma)
        return NULL;

    nvic_int_enable(TRCH_IRQ__TRCH_DMA_ABORT);
    nvic_int_enable(TRCH_IRQ__TRCH_DMA_EV0);
    return trch_dma;
}

void trch_dma_deinit()
{
    nvic_int_disable(TRCH_IRQ__TRCH_DMA_ABORT);
    nvic_int_disable(TRCH_IRQ__TRCH_DMA_EV0);

    dma_destroy(trch_dma);
}
