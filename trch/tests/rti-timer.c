#include "hwinfo.h"
#include "nvic.h"
#include "test-rti-timer.h"

#include "test.h"

extern struct rti_timer *trch_rti_timer;

int test_core_rti_timer()
{
    int rc;

    nvic_int_enable(TRCH_IRQ__RTI_TIMER);

    rc = test_rti_timer(RTI_TIMER_TRCH__BASE, &trch_rti_timer);
    if (rc)
        goto cleanup;

    rc = 0;
cleanup:
        nvic_int_disable(TRCH_IRQ__RTI_TIMER);
        return rc;
}
