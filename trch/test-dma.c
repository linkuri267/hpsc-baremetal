#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "dma.h"
#include "hwinfo.h"
#include "nvic.h"
#include "test.h"

static struct dma *trch_dma;

static uint8_t trch_dma_mcode[256]; // store in TRCH SRAM

static uint32_t dma_src_buf[128] __attribute__((aligned(DMA_MAX_BURST_BYTES)));
static uint32_t dma_dst_buf[128] __attribute__((aligned(DMA_MAX_BURST_BYTES)));

#if TEST_TRCH_DMA_CB
static void dma_tx_completed(void *arg, int rc)
{
    volatile bool *dma_tx_done = arg;
    printf("MAIN: DMA CB: done\r\n");
    *dma_tx_done = true;
}
#endif // TEST_TRCH_DMA_CB

int test_trch_dma()
{
    nvic_int_enable(TRCH_IRQ__TRCH_DMA_ABORT);
    nvic_int_enable(TRCH_IRQ__TRCH_DMA_EV0);

    trch_dma = dma_create("TRCH", TRCH_DMA_BASE,
                          trch_dma_mcode, sizeof(trch_dma_mcode));
    if (!trch_dma)
	return 1;

    for (unsigned i = 0; i < sizeof(dma_src_buf) / sizeof(dma_src_buf[0]); ++i)
        dma_src_buf[i] = 0xf00d0000 | i;
    dump_buf("DMA src", dma_src_buf, sizeof(dma_src_buf) / sizeof(dma_src_buf[0]));

#if TEST_TRCH_DMA_CB
    bool dma_done = false;
#endif

    struct dma_tx *dma_tx =
        dma_transfer(trch_dma, /* chan */ 0,
                     dma_src_buf, dma_dst_buf, sizeof(dma_dst_buf),
#if TEST_TRCH_DMA_CB
                     dma_tx_completed, &dma_done);
#else
                     NULL, NULL);
#endif
    if (!dma_tx)
        return 1;

    printf("Waiting for DMA tx to complete\r\n");
#if TEST_TRCH_DMA_CB
    while (!dma_done);
#else
    int rc = dma_wait(dma_tx);
    if (rc)
        return 1;
#endif
    printf("DMA tx completed\r\n");

    dump_buf("DMA dst", dma_dst_buf, sizeof(dma_dst_buf) / sizeof(dma_dst_buf[0]));
    for (unsigned i = 0; i < sizeof(dma_dst_buf) / sizeof(dma_dst_buf[0]); ++i) {
        if (dma_dst_buf[i] != dma_src_buf[i]) {
	    printf("DMA test: dest contents does not match src\r\n");
            return 1;
	}
    }

    dma_destroy(trch_dma);

    nvic_int_disable(TRCH_IRQ__TRCH_DMA_ABORT);
    nvic_int_disable(TRCH_IRQ__TRCH_DMA_EV0);

    return 0;
}

#define DMA_EV_ISR(dma, ev) \
	void dma_ ## dma ## _event_ ## ev ## _isr() { dma_event_isr(dma, ev); }

void dma_trch_dma_abort_isr() {
    dma_abort_isr(trch_dma);
}

DMA_EV_ISR(trch_dma, 0);
