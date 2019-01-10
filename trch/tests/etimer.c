#include "etimer.h"
#include "sleep.h"
#include "hwinfo.h"
#include "printf.h"
#include "nvic.h"

#include "test.h"

#define SYNC_INTERVAL_NS 1000000000

#define START_TIME_NS    5000000000
#define EVENT_DELTA_NS   1000000000

#define SYNC_INTERVAL_MS (SYNC_INTERVAL_NS / 1000000)
#define EVENT_DELTA_MS   (EVENT_DELTA_NS   / 1000000)

#define NUM_EVENTS 3

static void handle_event(struct etimer *et, void *arg)
{
    volatile int *events = arg;
    (*events)++;
    printf("ETMR test: events -> %u\r\n", *events);
}

int test_etimer()
{
    int rc = 1;
    int events = 0;

    struct etimer *et = etimer_create("ETMR", ETIMER__BASE,
            handle_event, &events,
            ETIMER_NOMINAL_FREQ_HZ, ETIMER_CLK_FREQ_HZ, ETIMER_MAX_DIVIDER);
    if (!et)
        return 1;

    nvic_int_enable(TRCH_IRQ__ELAPSED_TIMER);

    int ret = etimer_configure(et, ETIMER_MIN_CLK_FREQ_HZ, ETIMER_SYNC_SW, SYNC_INTERVAL_NS);
    if (ret)
        goto cleanup;

    etimer_load(et, START_TIME_NS);
    uint64_t count = etimer_capture(et);
    if (count < START_TIME_NS) {
        printf("TEST: FAIL: ETMR: current value below loaded value: "
               "0x%08x%08x < 0x%08x%08x\r\n",
               (uint32_t)(count >> 32), (uint32_t)count,
               (uint32_t)(START_TIME_NS >> 32), (uint32_t)START_TIME_NS);
        goto cleanup;
    }

    uint64_t count2 = etimer_capture(et);
    if (count2 <= count) {
        printf("TEST: FAIL: ETMR: value did not advance: "
               "0x%08x%08x <= 0x%08x%08x\r\n",
               (uint32_t)(count2 >> 32), (uint32_t)count2,
               (uint32_t)(count >> 32), (uint32_t)count);
        goto cleanup;
    }

    if (events > 0) {
        printf("TEST: FAIL: ETMR: unexpected events\r\n");
        goto cleanup;
    }

    for (int i = 1; i <= NUM_EVENTS; ++i) {
        etimer_event(et, etimer_capture(et) + EVENT_DELTA_NS);
        mdelay(EVENT_DELTA_MS);
        if (events != i) {
            printf("TEST: FAIL: ETMR: unexpected event count: %u != %u\r\n",
                   events, i);
            goto cleanup;
        }
    }

    // To test sync, we sample twice within one sync interval, but insert a
    // sync command in between. The sync should advance the timer to the next
    // multiple of the sync interval. So, the time between samples with sync in
    // between should be more than the sync interval, but without sync should
    // be less than sync interval.
    if (count2 - count >= SYNC_INTERVAL_NS) {
        printf("TEST: FAIL: ETMR: can't sample twice within one sync interval\r\n");
        goto cleanup;
    }

    count = etimer_capture(et);
    etimer_sync(et);
    etimer_sync(et);
    count2 = etimer_capture(et);

    if (count2 - count < SYNC_INTERVAL_NS) {
        printf("TEST: FAIL: ETMR: sync did not advance timer\r\n");
        goto cleanup;
    }

    etimer_sync(et);
    uint64_t skew = etimer_skew(et);
    if (skew > SYNC_INTERVAL_NS) {
        printf("TEST: FAIL: ETMR: skew after sync too high: 0x%08x%08x > 0x%x\r\n",
               (uint32_t)(skew >> 32), (uint32_t)skew, SYNC_INTERVAL_NS);
        goto cleanup;
    }
    mdelay(SYNC_INTERVAL_MS);
    skew = etimer_skew(et);
    if (skew < SYNC_INTERVAL_NS) {
        printf("TEST: FAIL: ETMR: skew after sync too low: %u < %u\r\n",
               skew, SYNC_INTERVAL_NS);
        goto cleanup;
    }

    rc = 0;
cleanup:
    nvic_int_disable(TRCH_IRQ__ELAPSED_TIMER);
    etimer_destroy(et);
    return rc;
}
