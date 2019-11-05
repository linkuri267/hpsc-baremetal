#include "hwinfo.h"
#include "gic.h"
#include "arm.h"
#include "subsys.h"
#include "panic.h"
#include "test-rti-timer.h"

#include "test.h"

int test_core_rti_timer(struct rti_timer **tmr_ptr)
{
    int rc;

    gic_int_enable(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_EDGE);

    // Each timer can be tested only from its associated core
    unsigned core = self_core_id();
    ASSERT(core < RTPS_R52_NUM_CORES);

    uintptr_t base = core ?
        RTI_TIMER_RTPS_R52_1__RTPS_BASE : RTI_TIMER_RTPS_R52_0__RTPS_BASE;

    rc = test_rti_timer(base, tmr_ptr);
    if (rc)
        goto cleanup;

    rc = 0;
cleanup:
    gic_int_disable(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI);
    return rc;
}
