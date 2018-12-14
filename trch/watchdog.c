#include <stdint.h>

#include "wdt.h"
#include "nvic.h"
#include "panic.h"
#include "reset.h"
#include "hwinfo.h"
#include "boot.h"

#include "watchdog.h"

// Must be global for the standalone tests for WDTs in test-wdt.c
struct wdt *trch_wdt = NULL;
struct wdt *rtps_wdts[RTPS_NUM_CORES] = {0};
struct wdt *hpps_wdts[HPPS_NUM_CORES] = {0};

// TODO: launch both timers, when we have both cores running and kicking
#undef RTPS_NUM_CORES
#define RTPS_NUM_CORES 1

#define NUM_STAGES 2 // fixed in HW, but flexible in driver

// IDs for CPU 0 in each subsystem (private to this file)
#define CPUID_TRCH 0
#define CPUID_RTPS 1
#define CPUID_HPPS 3

static void handle_timeout(struct wdt *wdt, unsigned stage, void *arg)
{
    unsigned cpuid = (unsigned)arg;
    unsigned subsys = SUBSYS_INVALID;
    struct wdt **wdts;
    unsigned num_cores;
    int rc, i;

    printf("watchdog: cpu %u: stage %u: expired\r\n", cpuid, stage);

    // Expand this with appropriate responses to faults
    switch (cpuid) {
        case CPUID_TRCH:
            ASSERT(stage == 0); // no last stage interrupt, because wired to hw reset
            // Note: main loop will return from WFI/WFE and kick too, but in case we
            // are busy with a long-running operation (e.g. loading data from SMC), then
            // main loop won't get a chance to run, so we kick it from here.
            //
            // Kicking on ST1 expiration is somewhat unsatisfactory: if we had
            // a scheduler and a systick, the systick would return control flow
            // to the scheduler loop, kick the WDT from that loop, and then
            // return to task (or context switch to another task). Even so, the
            // WDT would then be monitoring the scheduler, not whether any task
            // is stuck.
            wdt_kick(wdt);
            break;
        case CPUID_RTPS + 0:
        case CPUID_RTPS + 1:
            subsys = SUBSYS_RTPS;
            num_cores = RTPS_NUM_CORES;
            wdts = rtps_wdts;
            break;
        case CPUID_HPPS + 0:
        case CPUID_HPPS + 1:
        case CPUID_HPPS + 2:
        case CPUID_HPPS + 3:
        case CPUID_HPPS + 4:
        case CPUID_HPPS + 5:
        case CPUID_HPPS + 6:
        case CPUID_HPPS + 7:
            subsys = SUBSYS_HPPS;
            num_cores = HPPS_NUM_CORES;
            wdts = hpps_wdts;
            break;
        default:
	    ASSERT(false && "invalid context");
    }

    if (subsys != SUBSYS_INVALID) {
        ASSERT(stage == NUM_STAGES - 1); // first stage is handled by the target CPU
        ASSERT(!wdt_is_enabled(wdt)); // HW disables the timer on expiration
        wdt_kick(wdt); // to reload the timer, so that it's ready when subsystem enables it

        // The purpose of the following logic is to make the reset happen only
        // once, upon the expiration of any WDT timer from the set of timers of
        // a subsystem, instead of initiating a reset in response to the expiration
        // of every timer in the set. The only-once logic is somewhat subtle, because we
        // cannot know when the timers that were going to expire have expired and
        // thus we could start the reboot. We cannot wait for all timers in the
        // set to expire, becsause (1) the subsystem might not have enabled all
        // timers, and (2) the subsystem might be in a half-broken state where
        // it continues to kick some but not all timers.

        rc = reset_assert(subsys); // will prevent all CPUs from kicking
        if (rc) {
            printf("ERROR: WATCHDOG: failed to reset subsys %u\r\n",
                   subsys);
            return;
        }

        // Disable the WDTs for all other cores, so that they don't fire.  We
        // might still get an ISR from timers that already expired before we
        // disabled them, but that's ok: all of those ISRs (as well as the
        // first one), will collectively enqueue only one reboot request.  The
        // key is that by disabling all other WDTs from the first ISR, we
        // guarantee that the extra ISRs will happen back to back, i.e.
        // without entering the main loop, and thus without processing the
        // reboot request. So, when the main loop does run, it is safe to
        // initiate the reboot, since all timers will be disabled and reloaded.
        //
        // Note that the kick above will reload the counter of any timer that
        // was enabled, and for timers that were never enabled, the counter
        // does not need to be reloaded. (Instead of relying on this invariant,
        // we could reload timers on each boot, but this is equivalent.)
        for (i = 0; i < num_cores; ++i)
            wdt_disable(wdts[i]);

        // Enqueue the request for the main loop
        // Note: we cannot use the command queue because we want all ISRs
        // in a sequence to collectively generate one request (see above).
        boot_request_reboot(subsys);
    }
}

static struct wdt *create_wdt(const char *name, volatile uint32_t *base,
			      unsigned irq, uint64_t *timeouts, unsigned cpuid)
{
    struct wdt *wdt = wdt_create_monitor(name, base,
				         handle_timeout, (void *)cpuid);
    if (!wdt)
        return NULL;

    int rc = wdt_configure(wdt, WDT_MIN_FREQ_HZ, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(wdt);
	return NULL;
    }

    nvic_int_enable(irq);
    return wdt;
}

static void destroy_wdt(struct wdt *wdt, unsigned irq)
{
    // NOTE: order: ISR might be called during destroy if IRQ is not disabled
    nvic_int_disable(irq);
    wdt_disable(wdt);
    wdt_destroy(wdt);
}

int watchdog_trch_start() {
    uint64_t timeouts[] = { 50000000, 100000000 }; // about 10 sec for ST1 in wall-clock in Qemu
    trch_wdt = create_wdt("TRCH", WDT_TRCH_BASE, TRCH_IRQ__WDT_TRCH_ST1,
		      timeouts, CPUID_TRCH);
    if (!trch_wdt)
	return 1;
    wdt_enable(trch_wdt); // TRCH is the monitored target, so it enables
    return 0;
}

void watchdog_trch_stop() {
    destroy_wdt(trch_wdt, TRCH_IRQ__WDT_TRCH_ST1);
    trch_wdt = NULL;
}

void watchdog_trch_kick() {
    wdt_kick(trch_wdt);
}

int watchdog_rtps_start() {
    int i;
    uint64_t timeouts[] = { 10000000, 50000000 }; // about 1 sec, 5 sec

    static const char * const rtps_wdt_names[RTPS_NUM_CORES] = {
	"RTPS0", /* "RTPS1" */ // TODO: when we have both cores
    };

    for (i = 0; i < RTPS_NUM_CORES; ++i) {
	ASSERT(i < sizeof(rtps_wdt_names) / sizeof(rtps_wdt_names[0]));
	rtps_wdts[i] = create_wdt(rtps_wdt_names[i],
		(volatile uint32_t *)((volatile uint8_t *)WDT_RTPS_TRCH_BASE +
						          i * WDT_RTPS_SIZE),
		TRCH_IRQ__WDT_RTPS0_ST2 + i, timeouts, CPUID_RTPS + i);
	if (!rtps_wdts[i])
	    goto cleanup;
    }
    return 0;

cleanup:
    for (--i; i >= 0; --i) {
	destroy_wdt(rtps_wdts[i], TRCH_IRQ__WDT_RTPS0_ST2 + i);
	rtps_wdts[i] = NULL;
    }
    return 1;
}

void watchdog_rtps_stop() {
    for (int i = 0; i < RTPS_NUM_CORES;  ++i) {
	destroy_wdt(rtps_wdts[i], TRCH_IRQ__WDT_RTPS0_ST2 + i);
	rtps_wdts[i] = NULL;
    }
}

int watchdog_hpps_start() {
    int i;
    uint64_t timeouts[] = { 100000000, 200000000 };

    static const char * const hpps_wdt_names[HPPS_NUM_CORES] = {
	"HPPS0", "HPPS1", "HPPS2", "HPPS3",
	"HPPS4", "HPPS5", "HPPS6", "HPPS7"
    };

    for (i = 0; i < HPPS_NUM_CORES;  ++i) {
	ASSERT(i < sizeof(hpps_wdt_names) / sizeof(hpps_wdt_names[0]));
	hpps_wdts[i] = create_wdt(hpps_wdt_names[i],
		(volatile uint32_t *)((volatile uint8_t *)WDT_HPPS_TRCH_BASE +
							  i * WDT_HPPS_SIZE),
		TRCH_IRQ__WDT_HPPS0_ST2 + i, timeouts, CPUID_HPPS + i);
	if (!hpps_wdts[i])
	    goto cleanup;
    }
    return 0;

cleanup:
    for (--i; i >= 0; --i) {
	destroy_wdt(hpps_wdts[i], TRCH_IRQ__WDT_HPPS0_ST2 + i);
	hpps_wdts[i] = NULL;
    }
    return 1;
}

void watchdog_hpps_stop() {
    for (int i = 0; i < HPPS_NUM_CORES;  ++i) {
	destroy_wdt(hpps_wdts[i], TRCH_IRQ__WDT_HPPS0_ST2 + i);
	hpps_wdts[i] = NULL;
    }
}

void wdt_trch_st1_isr()
{
    wdt_isr(trch_wdt, /* stage */ 0);
}

#define WDT_ISR(sys, wdts, idx) void wdt_ ## sys ## idx ## _st2_isr() { wdt_isr(wdts[idx], 1); }

#define WDT_RTPS_ISR(idx) WDT_ISR(rtps, rtps_wdts, idx)
#define WDT_HPPS_ISR(idx) WDT_ISR(hpps, hpps_wdts, idx)

WDT_RTPS_ISR(0)
WDT_RTPS_ISR(1)

WDT_HPPS_ISR(0)
WDT_HPPS_ISR(1)
WDT_HPPS_ISR(2)
WDT_HPPS_ISR(3)
WDT_HPPS_ISR(4)
WDT_HPPS_ISR(5)
WDT_HPPS_ISR(6)
WDT_HPPS_ISR(7)
