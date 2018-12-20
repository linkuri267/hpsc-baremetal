#include "printf.h"
#include "gic.h"
#include "hwinfo.h"
#include "sleep.h"
#include "watchdog.h"
#include "wdt.h"

#include "test.h"

#define NUM_STAGES 2

#define WDT_FREQ_HZ WDT_MIN_FREQ_HZ // this has to match choice in TRCH

#define WDT_CYCLES_TO_MS(c) ((c) / (WDT_FREQ_HZ / 1000))

static void wdt_tick(struct wdt *wdt, unsigned stage, void *arg)
{
    volatile unsigned *expired = arg;
    printf("wdt test: expired\r\n");

    // convention in driver is zero-based, but here is one-based, because we
    // want 0 to indicate no stage expired
    *expired = stage + 1;
}

static bool check_expiration(unsigned expired_stage, unsigned expected)
{
    if (expired_stage != expected) {
        printf("wdt test: unexpected expired stage: %u != %u\r\n",
                expired_stage, expected);
        return false;
    }
    printf("wdt test: expected expired stage: %u == %u\r\n",
           expired_stage, expected);
    return true;
}

int test_wdt()
{
    int rc = 1;

    volatile unsigned expired_stage = 0;
    wdt = wdt_create_target("RTPS0", WDT_RTPS_RTPS_BASE + 0 * WDT_RTPS_SIZE,
                            wdt_tick, (void *)&expired_stage);
    if (!wdt)
        return 1;

    wdt_enable(wdt);

    gic_int_enable(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);

    unsigned timeouts[] = { wdt_timeout(wdt, 0), wdt_timeout(wdt, 1) };

    msleep(WDT_CYCLES_TO_MS(timeouts[0]));
    if (!check_expiration(expired_stage, 1)) goto cleanup;

    expired_stage = 0;

    unsigned total_timeout =
        WDT_CYCLES_TO_MS(timeouts[0]) + WDT_CYCLES_TO_MS(timeouts[1]);
    unsigned kick_interval = WDT_CYCLES_TO_MS(timeouts[0]) / 2;
    unsigned runtime = 0;
    while (runtime < total_timeout) {
        wdt_kick(wdt);
        msleep(kick_interval);
        if (!check_expiration(expired_stage, 0)) goto cleanup;
        runtime += kick_interval;
    }
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    rc = 0;

cleanup:
    // NOTE: order is important, since ISR may be called during destroy
    gic_int_disable(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI);
    wdt_destroy(wdt);
    wdt = NULL;
    // NOTE: timer is still running! target subsystem not allowed to disable it
    return rc;
}
