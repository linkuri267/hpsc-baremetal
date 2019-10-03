#include <stdbool.h>

#include "wdt.h"
#include "watchdog.h"
#include "printf.h"
#include "nvic.h"
#include "hwinfo.h"
#include "sleep.h"

#include "test.h"

#define WDT_FREQ_HZ WDT_MIN_FREQ_HZ // our choice for the test

#define INTERVAL_MS         1500 // shortest with sleep timer ticks at 0.5 sec
#define CHECK_INTERVAL_MS   (INTERVAL_MS + INTERVAL_MS / 4) // we want a check slightly later
#define INTERVAL_WDT_CYCLES (INTERVAL_MS * (WDT_FREQ_HZ / 1000))

#define NUM_STAGES 2

struct wdt_info {
    const char *name;

    // WDT instance base = group base + offset * as_size
    // Do this math at runtime, since the expressions need casts that make
    // them to long to write in the table
    uintptr_t base; // of the group
    unsigned offset; // from base
    unsigned as_size;

    unsigned irq;
};

static const struct wdt_info wdt_info[] = {
    { "TRCH",       WDT_TRCH_BASE,            0, WDT_TRCH_SIZE,     TRCH_IRQ__WDT_TRCH_ST1 },
    { "RTPS_R52_0", WDT_RTPS_R52_0_TRCH_BASE, 0, WDT_RTPS_R52_SIZE, TRCH_IRQ__WDT_RTPS_R52_0_ST2 },
    { "RTPS_R52_1", WDT_RTPS_R52_0_TRCH_BASE, 1, WDT_RTPS_R52_SIZE, TRCH_IRQ__WDT_RTPS_R52_1_ST2 }, /* _0_ not typo */
    { "RTPS_A53",   WDT_RTPS_A53_TRCH_BASE,   0, WDT_RTPS_A53_SIZE, TRCH_IRQ__WDT_RTPS_A53_ST2 },
    { "HPPS_0",     WDT_HPPS_TRCH_BASE,       0, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS0_ST2 },
    { "HPPS_1",     WDT_HPPS_TRCH_BASE,       1, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS1_ST2 },
    { "HPPS_2",     WDT_HPPS_TRCH_BASE,       2, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS2_ST2 },
    { "HPPS_3",     WDT_HPPS_TRCH_BASE,       3, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS3_ST2 },
    { "HPPS_4",     WDT_HPPS_TRCH_BASE,       4, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS4_ST2 },
    { "HPPS_5",     WDT_HPPS_TRCH_BASE,       5, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS5_ST2 },
    { "HPPS_6",     WDT_HPPS_TRCH_BASE,       6, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS6_ST2 },
    { "HPPS_7",     WDT_HPPS_TRCH_BASE,       7, WDT_HPPS_SIZE,     TRCH_IRQ__WDT_HPPS7_ST2 },
};
#define NUM_WDTS (sizeof(wdt_info) / sizeof(wdt_info[0]))

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

static int test_trch_wdt(struct wdt **trch_wdt_ptr)
{
    printf("TEST WDT: TRCH: interval %u ms\r\n", INTERVAL_MS);

    volatile unsigned expired_stage;
    struct wdt *trch_wdt = wdt_create_monitor("TRCH", WDT_TRCH_BASE,
                                   wdt_tick, (void *)&expired_stage,
                                   WDT_CLK_FREQ_HZ, WDT_MAX_DIVIDER);
    if (!trch_wdt)
        return 1;

    *trch_wdt_ptr = trch_wdt; // for ISR

    uint64_t timeouts[] = { INTERVAL_WDT_CYCLES, INTERVAL_WDT_CYCLES };
    int rc = wdt_configure(trch_wdt, WDT_FREQ_HZ, NUM_STAGES, timeouts);
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
    msleep(CHECK_INTERVAL_MS);
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
    total_timeout = total_timeout / (WDT_FREQ_HZ / 1000); // cycles to ms
    unsigned kick_interval = INTERVAL_MS / 4;
    while (runtime < total_timeout) {
        msleep(kick_interval);
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
    msleep(CHECK_INTERVAL_MS / 2);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup;
    wdt_disable(trch_wdt); // pause
    msleep(CHECK_INTERVAL_MS);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    if (!check_disabled(trch_wdt)) goto cleanup;
    wdt_enable(trch_wdt); // resume
    msleep(CHECK_INTERVAL_MS / 2); // second half should be enough for 1st stage to expire
    if (!check_expiration(expired_stage, 1)) goto cleanup;
    if (!check_enabled(trch_wdt)) goto cleanup;

    wdt_kick(trch_wdt); // to clear count from perious test
    expired_stage = 0;

    // Without kicking, but disabled -- expect no expiration
    printf("wdt test: (4) without kicking, disabled timer...\r\n");
    wdt_enable(trch_wdt);
    msleep(CHECK_INTERVAL_MS / 2);
    wdt_disable(trch_wdt);
    msleep(CHECK_INTERVAL_MS);
    if (!check_expiration(expired_stage, 0)) goto cleanup;
    msleep(CHECK_INTERVAL_MS);
    if (!check_expiration(expired_stage, 0)) goto cleanup;

    // NOTE: can't test Stage 2 expiration, because it's not wired to an IRQ
    // but instead is hard-widred to RESET.

    rc = 0;
cleanup:
    wdt_disable(trch_wdt);
    // TODO: order is important since ISR might run while destroying
    nvic_int_disable(TRCH_IRQ__WDT_TRCH_ST1);
    wdt_destroy(trch_wdt);
    *trch_wdt_ptr = NULL;
    return rc;
}

static int test_wdt(struct wdt **wdt_ptr, const char *name,
                    uintptr_t base, unsigned irq)
{
    printf("TEST WDT: %s: interval %u ms\r\n", name, INTERVAL_MS);

    int rc = 1;

    volatile unsigned expired_stage;
    uint64_t timeouts[] = { INTERVAL_WDT_CYCLES, INTERVAL_WDT_CYCLES };
    struct wdt *wdt = wdt_create_monitor(name, base,
                    wdt_tick, (void *)&expired_stage,
                    WDT_CLK_FREQ_HZ, WDT_MAX_DIVIDER);
    if (!wdt)
        return 1;
    *wdt_ptr = wdt; // for ISR

    rc = wdt_configure(wdt, WDT_FREQ_HZ, NUM_STAGES, timeouts);
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
    msleep(CHECK_INTERVAL_MS * 2);
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
    *wdt_ptr = NULL;
    return rc;
}

int test_wdts()
{
    int rc = test_trch_wdt(&wdts[0]);
    if (rc)
        return rc;

    for (unsigned i = 1 /* trch tested above */; i < NUM_WDTS;  ++i) {
        const struct wdt_info *wi = &wdt_info[i];
        rc |= test_wdt(&wdts[i] /* see ISR in watchdog.c */, wi->name,
                       wi->base + wi->offset * wi->as_size, wi->irq);
       if (rc)
           return rc;
    }
    return 0;
}
