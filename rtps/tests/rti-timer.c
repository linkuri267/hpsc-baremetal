#include "hwinfo.h"
#include "gic.h"
#include "test-rti-timer.h"

#include "test.h"

int test_core_rti_timer(struct rti_timer **tmr_ptr)
{
    int rc;

    gic_int_enable(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_EDGE);

    // Test only one timer, because each timer can be tested only from
    // its associated core, and this BM code is not SMP.
    rc = test_rti_timer(RTI_TIMER_RTPS_R52_0__RTPS_BASE, tmr_ptr);
    if (rc)
        goto cleanup;

    rc = 0;
cleanup:
    gic_int_disable(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI);
    return rc;
}
