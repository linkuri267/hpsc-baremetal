#include <stdbool.h>

#include "printf.h"
#include "dma.h"
#include "hwinfo.h"
#include "dram-map.h"
#include "intc.h"
#include "test.h"

struct dma *rtps_dma; // must be exposed for the ISR


#ifdef TEST_RTPS_DMA_CB
static void dma_tx_completed(void *arg, int rc)
{
    volatile bool *dma_tx_done = arg;
    printf("MAIN: DMA CB: done\r\n");
    *dma_tx_done = true;
}
#endif // TEST_RTPS_DMA_CB

int test_rtps_dma()
{
    intc_int_enable(RTPS_DMA_ABORT_IRQ, IRQ_TYPE_EDGE);
    intc_int_enable(RTPS_DMA_EV0_IRQ, IRQ_TYPE_EDGE);

    rtps_dma = dma_create("RTPS", RTPS_DMA_BASE,
                          (uint8_t *)RTPS_DMA_MCODE_ADDR, RTPS_DMA_MCODE_SIZE);
    if (!rtps_dma)
	return 1;

    uint32_t *dma_src_buf = (uint32_t *)RTPS_DMA_SRC_ADDR;
    uint32_t *dma_dst_buf = (uint32_t *)RTPS_DMA_DST_ADDR;
    unsigned words = RTPS_DMA_SIZE / sizeof(uint32_t);

    printf("DMA src: ");
    for (unsigned i = 0; i < words; ++i) {
        dma_src_buf[i] = 0xf00d0000 | i;
        printf("%x ", dma_src_buf[i]);
    }
    printf("\r\n");

#ifdef TEST_RTPS_DMA_CB
    bool dma_done = false;
#endif

    struct dma_tx *dma_tx =
        dma_transfer(rtps_dma, /* chan */ 0,
                     dma_src_buf, dma_dst_buf, RTPS_DMA_SIZE,
#ifdef TEST_RTPS_DMA_CB
                     dma_tx_completed, &dma_done);
#else
                     NULL, NULL);
#endif
    if (!dma_tx)
        return 1;

    printf("Waiting for DMA tx to complete\r\n");
#ifdef TEST_RTPS_DMA_CB
    while (!dma_done);
#else
    int rc = dma_wait(dma_tx);
    if (rc)
        return 1;
#endif
    printf("DMA tx completed\r\n");

    printf("DMA dst: ");
    for (unsigned i = 0; i < words; ++i) {
        printf("%x ", dma_dst_buf[i]);
        if (dma_dst_buf[i] != dma_src_buf[i])
            return 1;
    }
    printf("\r\n");

    if (dma_destroy(rtps_dma))
	return 1;

    intc_int_disable(RTPS_DMA_ABORT_IRQ);
    intc_int_disable(RTPS_DMA_EV0_IRQ);

    return 0;
}
