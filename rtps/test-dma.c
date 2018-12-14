#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "mem.h"
#include "dma.h"
#include "hwinfo.h"
#include "dram-map.h"
#include "gic.h"
#include "test.h"

#define RTPS_DMA_WORDS (RTPS_DMA_SIZE / sizeof(uint32_t))

struct dma *rtps_dma; // must be exposed for the ISR

#if TEST_RTPS_DMA_CB
static void dma_tx_completed(void *arg, int rc)
{
    volatile bool *dma_tx_done = arg;
    printf("MAIN: DMA CB: done\r\n");
    *dma_tx_done = true;
}
#endif // TEST_RTPS_DMA_CB

static bool cmp_buf(uint32_t *src, uint32_t *dst)
{
    for (unsigned i = 0; i < RTPS_DMA_WORDS; ++i)
        if (dst[i] != src[i])
            return false;
    return true;
}

static int do_copy(uint32_t *src, uint32_t *dst, unsigned sz)
{
#if TEST_RTPS_DMA_CB
    bool dma_done = false;
#endif

    struct dma_tx *dma_tx =
        dma_transfer(rtps_dma, /* chan */ 0,
                     src, dst, RTPS_DMA_SIZE,
#if TEST_RTPS_DMA_CB
                     dma_tx_completed, &dma_done);
#else
                     NULL, NULL);
#endif
    if (!dma_tx)
        return 1;

    printf("Waiting for DMA tx to complete\r\n");
#if TEST_RTPS_DMA_CB
    while (!dma_done);
#else
    int rc = dma_wait(dma_tx);
    if (rc)
        return 1;
#endif
    printf("DMA tx completed\r\n");

    return 0;
}

int test_rtps_dma()
{
    gic_int_enable(RTPS_IRQ__RTPS_DMA_ABORT, GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_EDGE);
    gic_int_enable(RTPS_IRQ__RTPS_DMA_EV0, GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_EDGE);

    rtps_dma = dma_create("RTPS", RTPS_DMA_BASE,
                          (uint8_t *)RTPS_DMA_MCODE_ADDR, RTPS_DMA_MCODE_SIZE);
    if (!rtps_dma)
	return 1;

    uint32_t *src_buf = (uint32_t *)RTPS_DMA_SRC_ADDR;
    uint32_t *dst_buf = (uint32_t *)RTPS_DMA_DST_ADDR;

    for (unsigned i = 0; i < RTPS_DMA_WORDS; ++i)
        src_buf[i] = 0xf00d0000 | i;
    dump_buf("src", src_buf, RTPS_DMA_WORDS);

    int rc = do_copy(src_buf, dst_buf, RTPS_DMA_SIZE);
    if (rc)
        return rc;
    dump_buf("dst", dst_buf, RTPS_DMA_WORDS);
    if (!cmp_buf(src_buf, dst_buf)) {
        printf("DMA test: dst data mismatches src\r\n");
        return 1;
    }

#if TEST_RTPS_MMU // Additional test of MMU remapping:
    printf("DMA test: via MMU\r\n");

    bzero(dst_buf, RTPS_DMA_SIZE);
    if (cmp_buf(src_buf, dst_buf)) {
        printf("DMA test: erase of dst buf failed\r\n");
        return 1;
    }

    // MMU maps RTPS_DMA_DST_REMAP_ADDR to RTPS_DMA_DST_ADDR
    rc = do_copy(src_buf, (uint32_t *)RTPS_DMA_DST_REMAP_ADDR, RTPS_DMA_SIZE);
    if (rc)
        return rc;
    dump_buf("dst", dst_buf, RTPS_DMA_WORDS);
    if (!cmp_buf(src_buf, dst_buf)) {
        printf("DMA test (remap): dst data mismatches src\r\n");
        return 1;
    }
#endif // TEST_RTPS_MMU

    dma_destroy(rtps_dma);

    gic_int_disable(RTPS_IRQ__RTPS_DMA_ABORT, GIC_IRQ_TYPE_SPI);
    gic_int_disable(RTPS_IRQ__RTPS_DMA_EV0, GIC_IRQ_TYPE_SPI);

    return 0;
}
