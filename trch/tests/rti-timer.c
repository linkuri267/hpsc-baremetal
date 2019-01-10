#include <stdint.h>

#include "printf.h"
#include "rti-timer.h"
#include "hwinfo.h"
#include "nvic.h"
#include "sleep.h"

#include "test.h"

#define INTERVAL_NS 1000000000
#define INTERVAL_MS (INTERVAL_NS / 1000000)

// Number of intervals to expect the ISR for.
//
// Note: can't be too high, because eventually our checks skew relative to the
// timer, since our sleep interval does not account for the loop overhead and
// because in the current sleep implementation the granularity of the sleep
// interval is at the granularity of the system timer, which can be low (which
// makes subtracting the loop overhead not possible).
#define NUM_EVENTS 3

extern struct rti_timer *trch_rti_timer;

static void handle_event(struct rti_timer *tmr, void *arg)
{
    volatile int *events = arg;
    (*events)++;
    printf("RTI TMR test: events -> %u\r\n", *events);
}

int test_rti_timer()
{
    int rc = 1;
    int events = 0;

    struct rti_timer *tmr = rti_timer_create("RTI TMR", RTI_TIMER__BASE,
            handle_event, &events);
    if (!tmr)
        return 1;
    trch_rti_timer = tmr; // for ISR

    nvic_int_enable(TRCH_IRQ__RTI_TIMER);

    uint64_t count = rti_timer_capture(tmr);
    mdelay(1);
    uint64_t count2 = rti_timer_capture(tmr);
    if (count2 <= count) {
        printf("TEST: FAIL: RTI TMR: value did not advance: "
               "0x%08x%08x <= 0x%08x%08x\r\n",
               (uint32_t)(count2 >> 32), (uint32_t)count2,
               (uint32_t)(count >> 32), (uint32_t)count);
        goto cleanup;
    }

    if (events > 0) {
        printf("TEST: FAIL: RTI TMR: unexpected events\r\n");
        goto cleanup;
    }

    rti_timer_configure(tmr, INTERVAL_NS);

    mdelay(INTERVAL_MS / 10); // offset the checks by an epsilon
    for (int i = 1; i <= NUM_EVENTS; ++i) {
        mdelay(INTERVAL_MS);
        if (events != i) {
            printf("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\r\n",
                   events, i);
            goto cleanup;
        }
    }

    rc = 0;
cleanup:
    nvic_int_disable(TRCH_IRQ__RTI_TIMER);
    rti_timer_destroy(tmr);
    return rc;
}
