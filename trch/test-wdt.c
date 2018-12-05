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
        printf("ERROR: wdt test: unexpected expired stage: %u != %u\r\n",
                expired_stage, expected);
        return false;
    }
    printf("wdt test: correct expired stage: %u == %u\r\n",
           expired_stage, expected);
    return true;
}

static bool check_enabled(struct wdt *wdt)
{
    if (!wdt_is_enabled(wdt)) {
        printf("ERROR: wdt test: unexpected timer state: disabled\r\n");
        return false;
    }
    printf("wdt test: correct timer state: enabled\r\n");
    return true;
}

static bool check_disabled(struct wdt *wdt)
{
    if (wdt_is_enabled(wdt)) {
        printf("ERROR: wdt test: unexpected timer state: enabled\r\n");
        return false;
    }
    printf("wdt test: correct timer state: disabled\r\n");
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
    int rc = wdt_configure(trch_wdt, WDT_MIN_FREQ_HZ, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(trch_wdt);
        return rc;
    }
    
    nvic_int_enable(TRCH_IRQ__WDT_TRCH_ST1);

    rc = 1;

    // Without kicking -- expect expiration of each stage in sequence
    printf("wdt test: (1) without kicking...\r\n");
    expired_stage = 0;
    wdt_enable(trch_wdt);
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 1)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup; // should stay enabled
 
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
        if (!check_enabled(trch_wdt)) goto cleanup; // should stay enabled
        wdt_kick(trch_wdt);
        runtime += kick_interval;
        printf("wdt test: kick: runtime %u / total %u\r\n", runtime, total_timeout);
    }
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup; // should stay enabled

    wdt_kick(trch_wdt); // to clear count from perious test
    expired_stage = 0;

    printf("wdt test: (3) without kicking, pause resume...\r\n");
    wdt_enable(trch_wdt);
    delay(INTERVAL / 2);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup;
    wdt_disable(trch_wdt); // pause
    delay(INTERVAL);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!check_disabled(trch_wdt)) goto cleanup;
    wdt_enable(trch_wdt); // resume
    delay(INTERVAL / 2); // second half should be enough for 1st stage to expire
    if (!check_expiration(expired_stage, 1)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup;

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

    // NOTE: can't test Stage 2 expiration, because it's not wired to an IRQ
    // but instead is hard-widred to RESET.

    rc = 0;
cleanup:
    wdt_disable(trch_wdt);
    // TODO: order is important since ISR might run while destroying
    nvic_int_disable(TRCH_IRQ__WDT_TRCH_ST1);
    wdt_destroy(trch_wdt);
    return rc;
}
#endif // TEST_TRCH_WDT_STANDALONE

#if TEST_RTPS_WDT_STANDALONE || TEST_HPPS_WDT_STANDALONE
int test_wdt(struct wdt **wdt_ptr, const char *name,
             volatile uint32_t *base, unsigned irq)
{
    int rc = 1;

    volatile unsigned expired_stage;
    uint64_t timeouts[] = { INTERVAL, INTERVAL };
    struct wdt *wdt = wdt_create_monitor(name, base,
                    wdt_tick, (void *)&expired_stage);
    if (!wdt)
        return 1;
    *wdt_ptr = wdt; // for ISR

    rc = wdt_configure(wdt, WDT_MIN_FREQ_HZ, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(wdt);
	return rc;
    }

    nvic_int_enable(irq);

    rc = 1;

    // Do not reset the target core, so expect timer expiration
    expired_stage = 0;
    wdt_enable(wdt);
    if (!check_enabled(wdt)) goto cleanup;
    delay(INTERVAL * 2);
    if (!check_expiration(expired_stage, 2)) goto cleanup;
    if (!check_disabled(wdt)) goto cleanup;

    // NOTE: Can't test Stage 1 expiration because the first stage generates
    // only a private IRQ to the target core.

    // The test where the target core does kick is done as part of the WDT test
    // in the main loop, that represents a representative setup of WDT.

    rc = 0;
cleanup:
    wdt_disable(wdt);
    // TODO: order is important since ISR might run while destroying
    nvic_int_disable(irq);
    wdt_destroy(wdt);
    return rc;
}
#endif // TEST_RTPS_WDT_STANDALONE || TEST_HPPS_WDT_STANDALONE

#if TEST_RTPS_WDT_STANDALONE
int test_rtps_wdt()
{
     static const char * const rtps_wdt_names[HPPS_NUM_CORES] = {
         "RTPS0", "RTPS1"
     };
     for (int core = 0; core < RTPS_NUM_CORES; ++core) {
	     int rc = test_wdt(&rtps_wdts[core], rtps_wdt_names[core],
		(volatile uint32_t *)((volatile uint8_t *)WDT_RTPS_TRCH_BASE +
                                                          core * WDT_RTPS_SIZE),
		TRCH_IRQ__WDT_RTPS0_ST2 + core);
	     if (rc)
		return rc;
     }
     return 0;
}
#endif // TEST_RTPS_WDT_STANDALONE

#if TEST_HPPS_WDT_STANDALONE
int test_hpps_wdt()
{
     static const char * const hpps_wdt_names[HPPS_NUM_CORES] = {
         "HPPS0", "HPPS1", "HPPS2", "HPPS3",
         "HPPS4", "HPPS5", "HPPS6", "HPPS7"
     };
     for (int core = 0; core < HPPS_NUM_CORES; ++core) {
	     int rc = test_wdt(&hpps_wdts[core], hpps_wdt_names[core],
		(volatile uint32_t *)((volatile uint8_t *)WDT_HPPS_TRCH_BASE +
                                                          core * WDT_HPPS_SIZE),
		TRCH_IRQ__WDT_HPPS0_ST2 + core);
	     if (rc)
		return rc;
     }
     return 0;
}
#endif // TEST_HPPS_WDT_STANDALONE
