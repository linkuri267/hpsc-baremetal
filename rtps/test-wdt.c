#include "printf.h"
#include "gic.h"
#include "hwinfo.h"
#include "delay.h"

#include "wdt.h"

struct wdt *wdt; // must be in global scope since needed by global ISR

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
    gic_int_enable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);
    volatile unsigned expired_stage;
    wdt = wdt_create("RTPS0", WDT_RTPS0_RTPS_BASE,
                                 wdt_tick, (void *)&expired_stage);

    unsigned total_timeout = 0;
    unsigned timeouts[] = { wdt_timeout(wdt, 0), wdt_timeout(wdt, 1) };
    total_timeout += timeouts[0] + timeouts[1];

    delay(timeouts[0]);
    if (!check_expiration(expired_stage, 1)) goto cleanup;

    unsigned kick_interval = timeouts[0] / 2;
    unsigned runtime = 0;
    while (runtime < total_timeout) {
        wdt_kick(wdt);
        delay(kick_interval);
        if (!check_expiration(expired_stage, 0)) goto cleanup;
        runtime += kick_interval;
    }
    if (!check_expiration(expired_stage, 0)) goto cleanup;

cleanup:
    wdt_destroy(wdt);
    gic_int_disable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI);
}
