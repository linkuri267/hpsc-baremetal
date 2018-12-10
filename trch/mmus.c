#include "hwinfo.h"
#include "dram-map.h"
#include "mailbox.h"
#include "mmu.h"

#include "mmus.h"

static struct mmu *rt_mmu;
static struct mmu_context *trch_ctx;
static struct mmu_context *rtps_ctx;
static struct mmu_stream *trch_stream;
static struct mmu_stream *rtps_stream;
static struct balloc *ba;

int rt_mmu_init()
{
    rt_mmu = mmu_create("RTPS/TRCH->HPPS", RTPS_TRCH_TO_HPPS_SMMU_BASE);
    if (!rt_mmu)
	return 1;

    ba = balloc_create("RT", (uint64_t *)RTPS_HPPS_PT_ADDR, RTPS_HPPS_PT_SIZE);
    if (!ba)
        goto cleanup_balloc;

    trch_ctx = mmu_context_create(rt_mmu, ba, MMU_PAGESIZE_4KB);
    if (!trch_ctx)
	goto cleanup_trch_context;

    rtps_ctx = mmu_context_create(rt_mmu, ba, MMU_PAGESIZE_4KB);
    if (!rtps_ctx)
	goto cleanup_rtps_context;

    trch_stream = mmu_stream_create(MASTER_ID_TRCH_CPU, trch_ctx);
    if (!trch_stream)
	goto cleanup_trch_stream;

    rtps_stream = mmu_stream_create(MASTER_ID_RTPS_CPU0, rtps_ctx);
    if (!rtps_stream)
	goto cleanup_rtps_stream;

    if (mmu_map(trch_ctx, (uint32_t)MBOX_HPPS_TRCH__BASE,
			  (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE))
        goto cleanup_trch_mailbox;
    if (mmu_map(rtps_ctx, (uint32_t)MBOX_HPPS_RTPS__BASE,
			  (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE))
        goto cleanup_rtps_mailbox;

    if (mmu_map(trch_ctx, HPPS_ATF_ADDR, HPPS_ATF_ADDR, HPPS_ATF_SIZE))
        goto cleanup_hpps_atf;
    if (mmu_map(trch_ctx, HPPS_BL_ADDR, HPPS_BL_ADDR, HPPS_BL_SIZE))
        goto cleanup_hpps_bl;
    if (mmu_map(trch_ctx, HPPS_DT_ADDR, HPPS_DT_ADDR, HPPS_DT_SIZE))
        goto cleanup_hpps_dt;
    if (mmu_map(trch_ctx, HPPS_KERN_ADDR, HPPS_KERN_ADDR, HPPS_KERN_SIZE))
        goto cleanup_hpps_kern;
    if (mmu_map(trch_ctx, HPPS_ROOTFS_ADDR, HPPS_ROOTFS_ADDR, HPPS_ROOTFS_SIZE))
        goto cleanup_hpps_rootfs;

    mmu_enable(rt_mmu);
    return 0;

cleanup_hpps_rootfs:
    mmu_unmap(trch_ctx, HPPS_KERN_ADDR, HPPS_KERN_SIZE);
cleanup_hpps_kern:
    mmu_unmap(trch_ctx, HPPS_DT_ADDR, HPPS_DT_SIZE);
cleanup_hpps_dt:
    mmu_unmap(trch_ctx, HPPS_BL_ADDR, HPPS_BL_SIZE);
cleanup_hpps_bl:
    mmu_unmap(trch_ctx, HPPS_ATF_ADDR, HPPS_ATF_SIZE);
cleanup_hpps_atf:
    mmu_unmap(rtps_ctx, (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE);
cleanup_rtps_mailbox:
    mmu_unmap(trch_ctx, (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE);
cleanup_trch_mailbox:
    mmu_stream_destroy(rtps_stream);
cleanup_rtps_stream:
    mmu_stream_destroy(trch_stream);
cleanup_trch_stream:
    mmu_context_destroy(rtps_ctx);
cleanup_rtps_context:
    mmu_context_destroy(trch_ctx);
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

    rc |= mmu_unmap(trch_ctx, HPPS_ATF_ADDR, HPPS_ATF_SIZE);
    rc |= mmu_unmap(trch_ctx, HPPS_BL_ADDR, HPPS_BL_SIZE);
    rc |= mmu_unmap(trch_ctx, HPPS_DT_ADDR, HPPS_DT_SIZE);
    rc |= mmu_unmap(trch_ctx, HPPS_KERN_ADDR, HPPS_KERN_SIZE);
    rc |= mmu_unmap(trch_ctx, HPPS_ROOTFS_ADDR, HPPS_ROOTFS_SIZE);

    rc |= mmu_unmap(rtps_ctx, (uint32_t)MBOX_HPPS_RTPS__BASE, HPSC_MBOX_AS_SIZE);
    rc |= mmu_unmap(trch_ctx, (uint32_t)MBOX_HPPS_TRCH__BASE, HPSC_MBOX_AS_SIZE);

    rc |= mmu_stream_destroy(rtps_stream);
    rc |= mmu_stream_destroy(trch_stream);
    rc |= mmu_context_destroy(rtps_ctx);
    rc |= mmu_context_destroy(trch_ctx);
    balloc_destroy(ba);
    rc |= mmu_destroy(rt_mmu);
    return rc;
}
