#include <stdbool.h>

#include "wdt.h"
#include "watchdog.h"
#include "printf.h"
#include "nvic.h"
#include "hwinfo.h"
#include "delay.h"

#include "test.h"

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

#if TEST_TRCH_WDT_STANDALONE
int test_trch_wdt()
{
    volatile unsigned expired_stage;
    trch_wdt = wdt_create_monitor("TRCH", WDT_TRCH_BASE,
                                  wdt_tick, (void *)&expired_stage);

    if (!trch_wdt)
        return 1;

    uint64_t timeouts[] = { INTERVAL, INTERVAL };
    int rc = wdt_configure(trch_wdt, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(trch_wdt);
        return rc;
    }
    
    nvic_int_enable(WDT_TRCH_ST1_IRQ);
    nvic_int_enable(WDT_TRCH_ST2_IRQ);

    rc = 1;

    // Without kicking -- expect expiration of each stage in sequence
    printf("wdt test: (1) without kicking...\r\n");
    expired_stage = 0;
    wdt_enable(trch_wdt);
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 1)) goto cleanup;
    if (!wdt_is_enabled(trch_wdt)) goto cleanup; // should stay enabled
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 2)) goto cleanup;
    if (wdt_is_enabled(trch_wdt)) goto cleanup; // shoudl disable itself
 
    wdt_kick(trch_wdt); // to clear count from perious test
    expired_stage = 0;

    // With kicking -- expect no expiration
    printf("wdt test: (2) with kicking...\r\n");
    wdt_enable(trch_wdt);
    unsigned runtime = 0, total_timeout = 0;
    for (unsigned i = 0; i < NUM_STAGES; ++i)
        total_timeout += timeouts[i];
    unsigned kick_interval = INTERVAL / 4;
    while (runtime < total_timeout) {
        delay(kick_interval);
        if (!check_expiration(expired_stage, 0)) goto cleanup;
        if (!wdt_is_enabled(trch_wdt)) goto cleanup; // should stay enabled
        wdt_kick(trch_wdt);
        runtime += kick_interval;
        printf("wdt test: kick: runtime %u / total %u\r\n", runtime, total_timeout);
    }
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!wdt_is_enabled(trch_wdt)) goto cleanup; // should stay enabled

    // Without kicking -- expect expiration of each stage in sequence
    printf("wdt test: (3) without kicking...\r\n");
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 1)) goto cleanup;
    if (!wdt_is_enabled(trch_wdt)) goto cleanup; // should stay enabled
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 2)) goto cleanup;
    if (wdt_is_enabled(trch_wdt)) goto cleanup; // should disable itself

    wdt_kick(trch_wdt); // to clear count from perious test
    expired_stage = 0;

    // Without kicking, but disabled -- expect no expiration
    printf("wdt test: (4) without kicking, disabled timer...\r\n");
    wdt_enable(trch_wdt);
    delay(INTERVAL / 2);
    wdt_disable(trch_wdt);
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 0)) goto cleanup;

    rc = 0;
cleanup:
    wdt_disable(trch_wdt);
    // TODO: order is important since ISR might run while destroying
    nvic_int_disable(WDT_TRCH_ST1_IRQ);
    nvic_int_disable(WDT_TRCH_ST2_IRQ);
    wdt_destroy(trch_wdt);
    return rc;
}
#endif // TEST_TRCH_WDT_STANDALONE

#if TEST_RTPS_WDT_STANDALONE
int test_rtps_wdt()
{
    // TODO: test RTPS1 too once we have the second core booting (in SPLIT mode?)
    int rc = 1;

    volatile unsigned expired_stage;
    uint64_t timeouts[] = { INTERVAL, INTERVAL };
    rtps_wdt = wdt_create_monitor("RTPS0", WDT_RTPS0_TRCH_BASE,
                                  wdt_tick, (void *)&expired_stage);
    if (!rtps_wdt)
        return 1;

    rc = wdt_configure(rtps_wdt, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(rtps_wdt);
	return rc;
    }

    nvic_int_enable(WDT_RTPS0_ST2_IRQ);

    rc = 1;

    // Do not reset RTPS, expect timer expiration
    expired_stage = 0;
    wdt_enable(rtps_wdt);
    delay(INTERVAL * 2);
    if (!check_expiration(expired_stage, 2)) goto cleanup;
    if (wdt_is_enabled(rtps_wdt)) goto cleanup;

    // The test where RTPS does kick is done as part of the WDT test
    // in the main loop, that represents a representative setup of WDT.

    rc = 0;
cleanup:
    wdt_disable(rtps_wdt);
    // TODO: order is important since ISR might run while destroying
    nvic_int_disable(WDT_RTPS0_ST2_IRQ);
    wdt_destroy(rtps_wdt);
    return rc;
}
#endif // TEST_RTPS_WDT_STANDALONE
