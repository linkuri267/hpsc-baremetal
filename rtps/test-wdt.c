#include "printf.h"
#include "gic.h"
#include "hwinfo.h"
#include "delay.h"
#include "watchdog.h"

#include "wdt.h"

#define INTERVAL 5000000 // cycles (about 2 sec wall-clock in Qemu)
#define NUM_STAGES 2

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
    wdt = wdt_create_target("RTPS0", WDT_RTPS0_RTPS_BASE,
                            wdt_tick, (void *)&expired_stage);
    if (!wdt)
        return 1;

    if (!wdt_is_enabled(wdt)) {
        printf("ERROR: wdt test: TRCH did not setup the WDT\r\n");
        wdt_destroy(wdt);
        return 1;
    }

    gic_int_enable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);

    unsigned total_timeout = 0;
    unsigned timeouts[] = { wdt_timeout(wdt, 0), wdt_timeout(wdt, 1) };
    total_timeout += timeouts[0] + timeouts[1];

    delay(timeouts[0]);
    if (!check_expiration(expired_stage, 1)) goto cleanup;

    expired_stage = 0;

    unsigned kick_interval = timeouts[0] / 2;
    unsigned runtime = 0;
    while (runtime < total_timeout) {
        wdt_kick(wdt);
        delay(kick_interval);
        if (!check_expiration(expired_stage, 0)) goto cleanup;
        runtime += kick_interval;
    }
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    rc = 0;

cleanup:
    // NOTE: order is important, since ISR may be called during destroy
    gic_int_disable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI);
    wdt_destroy(wdt);
    wdt = NULL;
    return rc;
}
