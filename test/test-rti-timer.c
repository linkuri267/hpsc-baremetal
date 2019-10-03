#include <stdint.h>

#include "printf.h"
#include "rti-timer.h"
#include "hwinfo.h"
#include "nvic.h"
#include "sleep.h"

#include "test-rti-timer.h"

#define INTERVAL_NS 1000000000
#define INTERVAL_MS (INTERVAL_NS / 1000000)

// Number of intervals to expect the ISR for.
#define NUM_EVENTS 3

static void handle_event(struct rti_timer *tmr, void *arg)
{
    volatile int *events = arg;
    (*events)++;
    printf("RTI TMR test: events -> %u\r\n", *events);
}

int test_rti_timer(uintptr_t base, struct rti_timer **tmr_ptr)
{
    int rc = 1;
    int events = 0;

    struct rti_timer *tmr = rti_timer_create("RTI TMR", base,
            handle_event, &events);
    if (!tmr)
        return 1;
    *tmr_ptr = tmr; // for ISR

    uint64_t count = rti_timer_capture(tmr);
    msleep(1);
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

    msleep(INTERVAL_MS / 10); // offset the checks by an epsilon
    for (int i = 1; i <= NUM_EVENTS; ++i) {
        msleep(INTERVAL_MS);
        if (events != i) {
            printf("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\r\n",
                   events, i);
            goto cleanup;
        }
    }

    // Since there's no way to disable the RTI timer, the best we can do to
    // reduce the load in the HW emulator, set the interval to max.
    rti_timer_configure(tmr, RTI_MAX_COUNT);

    rc = 0;
cleanup:
    rti_timer_destroy(tmr);
    *tmr_ptr = NULL;
    return rc;
}
