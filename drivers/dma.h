#ifndef DMA_H
#define DMA_H

#include <stdint.h>

#define DMA_MAX_BURST_BITS  8 // property of HW, see Table 3-21
#define DMA_MAX_BURST_BYTES (1 << DMA_MAX_BURST_BITS)

struct dma;
struct dma_tx;

typedef void (*dma_cb_t)(void *arg, int rc);

struct dma *dma_create(const char *name, volatile uint32_t *base,
                       uint8_t *mcode_addr, unsigned mcode_sz);
void dma_destroy(struct dma *dma);

// If callback is NULL, then must reap with dma_wait
// TODO: split into compilation and launching methods
struct dma_tx *dma_transfer(struct dma *dma, unsigned chan,
                            uint32_t *src, uint32_t *dst, unsigned sz,
                            dma_cb_t cb, void *cb_arg);
int dma_wait(struct dma_tx *tx);

void dma_abort_isr(struct dma *dma);
void dma_event_isr(struct dma *dma, unsigned ev);

#endif // DMA_H
