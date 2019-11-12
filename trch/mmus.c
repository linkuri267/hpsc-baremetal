#include "hwinfo.h"
#include "mem-map.h"
#include "mailbox.h"
#include "mmu.h"
#include "panic.h"

#include "mmus.h"

#if MMU_TEST_REGION_SIZE > RT_MMU_TEST_DATA_LO_SIZE
#error MMU_TEST_REGION greater than scratch space RT_MMU_TEST_DATA_LO_SIZE
#endif

static struct mmu *rt_mmu;
static struct mmu_context *ctx;
static struct mmu_stream *trch_stream;
static struct mmu_stream *rtps_r52_0_stream, *rtps_r52_1_stream;
static struct mmu_stream *rtps_a53_stream;
static struct balloc *ba;

int rt_mmu_init()
{
    rt_mmu = mmu_create("RTPS/TRCH->HPPS", RTPS_TRCH_TO_HPPS_SMMU_BASE);
    if (!rt_mmu)
	return 1;

    mmu_disable(rt_mmu); // might be already enabled if the core reboots

    ba = balloc_create("RT", (uint64_t *)RTPS_HPPS_PT_ADDR, RTPS_HPPS_PT_SIZE);
    if (!ba)
        goto cleanup_balloc;

    ctx = mmu_context_create(rt_mmu, ba, MMU_PAGESIZE_4KB);
    if (!ctx)
	goto cleanup_trch_context;

    trch_stream = mmu_stream_create(MMU_SID_NIC5, ctx);
    if (!trch_stream) goto cleanup_trch_stream;
    rtps_r52_0_stream = mmu_stream_create(MASTER_ID_RTPS_CPU0, ctx);
    if (!rtps_r52_0_stream) goto cleanup_rtps_r52_0_stream;
    rtps_r52_1_stream = mmu_stream_create(MASTER_ID_RTPS_CPU1, ctx);
    if (!rtps_r52_1_stream) goto cleanup_rtps_r52_1_stream;
    rtps_a53_stream = mmu_stream_create(MASTER_ID_RTPS_A53, ctx);
    if (!rtps_a53_stream) goto cleanup_rtps_a53_stream;

    if (mmu_map(ctx, (uint32_t)MBOX_HPPS_TRCH__BASE,
			  (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE))
        goto cleanup_trch_mailbox;

    if (mmu_map(ctx, (uint32_t)MBOX_HPPS_RTPS__BASE,
			  (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE))
        goto cleanup_rtps_mailbox;

    if (mmu_map(ctx, (uint32_t)HSIO_BASE, (uint32_t)HSIO_BASE, HSIO_SIZE))
        goto cleanup_trch_hsio;

    if (mmu_map(ctx, HPPS_DDR_LOW_ADDR, HPPS_DDR_LOW_ADDR,
                          HPPS_DDR_LOW_SIZE))
        goto cleanup_trch_hpps_ddr_low;

#if CONFIG_HPPS_TRCH_SHMEM || CONFIG_HPPS_TRCH_SHMEM_SSW
    if (mmu_map(ctx, HPPS_SHM_ADDR, HPPS_SHM_ADDR, HPPS_SHM_SIZE))
        goto cleanup_hpps_shmem;
#endif /* CONFIG_HPPS_TRCH_SHMEM */

#if TEST_RT_MMU
    if (mmu_map(ctx, RT_MMU_TEST_DATA_HI_0_WIN_ADDR, RT_MMU_TEST_DATA_HI_0_ADDR,
                          RT_MMU_TEST_DATA_HI_SIZE))
	goto cleanup_hi1_win;
    if (mmu_map(ctx, RT_MMU_TEST_DATA_HI_1_WIN_ADDR, RT_MMU_TEST_DATA_HI_1_ADDR,
                          RT_MMU_TEST_DATA_HI_SIZE))
	goto cleanup_hi0_win;
    if (mmu_map(ctx, RT_MMU_TEST_DATA_LO_ADDR,  RT_MMU_TEST_DATA_LO_ADDR,
                          RT_MMU_TEST_DATA_LO_SIZE))
	goto cleanup_lo_win;
#endif // TEST_RT_MMU

#if TEST_RIO
    if (mmu_map(ctx, RIO_MEM_WIN_ADDR, RIO_MEM_ADDR, RIO_MEM_SIZE))
        goto cleanup_rio_win;
#endif // TEST_RIO

    mmu_enable(rt_mmu);
    return 0;

#if TEST_RIO
cleanup_rio_win:
    mmu_unmap(ctx, RT_MMU_TEST_DATA_LO_ADDR, RT_MMU_TEST_DATA_LO_SIZE);
#endif // TEST_RIO
#if TEST_RT_MMU
cleanup_lo_win:
    mmu_unmap(ctx, RT_MMU_TEST_DATA_HI_1_WIN_ADDR, RT_MMU_TEST_DATA_HI_SIZE);
cleanup_hi0_win:
    mmu_unmap(ctx, RT_MMU_TEST_DATA_HI_0_WIN_ADDR, RT_MMU_TEST_DATA_HI_SIZE);
cleanup_hi1_win:
#endif // TEST_RT_MMU
#if CONFIG_HPPS_TRCH_SMMEM || CONFIG_HPPS_TRCH_SHMEM_SSW
    mmu_unmap(ctx, HPPS_SHM_ADDR, HPPS_SHM_SIZE);
cleanup_hpps_shmem:
#endif /* CONFIG_HPPS_TRCH_SMMEM || CONFIG_HPPS_TRCH_SHMEM_SSW */
    mmu_unmap(ctx, HPPS_DDR_LOW_ADDR, HPPS_DDR_LOW_SIZE);
cleanup_trch_hpps_ddr_low:
    mmu_unmap(ctx, (uint32_t)HSIO_BASE, HSIO_SIZE);
cleanup_trch_hsio:
    mmu_unmap(ctx, (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE);
cleanup_rtps_mailbox:
    mmu_unmap(ctx, (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE);
cleanup_trch_mailbox:
    mmu_stream_destroy(rtps_a53_stream);
cleanup_rtps_a53_stream:
    mmu_stream_destroy(rtps_r52_1_stream);
cleanup_rtps_r52_1_stream:
    mmu_stream_destroy(rtps_r52_0_stream);
cleanup_rtps_r52_0_stream:
    mmu_stream_destroy(trch_stream);
cleanup_trch_stream:
    mmu_context_destroy(ctx);
cleanup_trch_context:
    balloc_destroy(ba);
cleanup_balloc:
    mmu_destroy(rt_mmu);
    return 1;
}

int rt_mmu_deinit()
{
    int rc = 0;
    mmu_disable(rt_mmu);

#if TEST_RIO
    rc |= mmu_unmap(ctx, RIO_MEM_WIN_ADDR, RIO_MEM_SIZE);
    rc |= mmu_unmap(ctx, RT_MMU_TEST_DATA_LO_ADDR, RT_MMU_TEST_DATA_LO_SIZE);
#endif /* TEST_RIO */

    rc |= mmu_unmap(ctx, HPPS_DDR_LOW_ADDR, HPPS_DDR_LOW_SIZE);
    rc |= mmu_unmap(ctx, (uint32_t)HSIO_BASE, HSIO_SIZE);
    rc |= mmu_unmap(ctx, (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE);
    rc |= mmu_unmap(ctx, (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE);

    rc |= mmu_stream_destroy(trch_stream);
    rc |= mmu_context_destroy(ctx);
    balloc_destroy(ba);
    rc |= mmu_destroy(rt_mmu);
    return rc;
}

int rt_mmu_map(uint64_t vaddr, uint64_t paddr, unsigned sz)
{
    ASSERT(ctx);
    ASSERT(ALIGNED64(vaddr, 16)); /* 64k pages, to be safe */
    ASSERT(ALIGNED64(paddr, 16));
    return mmu_map(ctx, vaddr, paddr, sz);
}

int rt_mmu_unmap(uint64_t vaddr, unsigned sz)
{
    ASSERT(ctx);
    return mmu_unmap(ctx, vaddr, sz);
}
