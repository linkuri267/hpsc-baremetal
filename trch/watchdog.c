#include <stdint.h>

#include "wdt.h"
#include "nvic.h"
#include "panic.h"
#include "reset.h"
#include "hwinfo.h"

#include "watchdog.h"

// Must be global for the standalone tests for WDTs in test-wdt.c
struct wdt *rtps_wdt;
struct wdt *trch_wdt;

#define NUM_STAGES 2 // fixed in HW, but flexible in driver

// IDs for CPU 0 in each subsystem (private to this file)
#define CPUID_TRCH 0
#define CPUID_RTPS 1
#define CPUID_HPPS 3

static void handle_timeout(struct wdt *wdt, unsigned stage, void *arg)
{
    unsigned cpuid = (unsigned)arg;
    printf("watchdog: cpu %u: expired\r\n", cpuid);

    // Expand this with appropriate responses to faults
    switch (cpuid) {
        case CPUID_TRCH:
            // ASSERT(stage == 0); // TODO: no last stage interrupt, because hw reset
            if (stage == 0) {
		// nothing to do: main loop will return from WFI/WFE and kick
            } else {
                panic("TRCH watchdog expired\r\n");
            }
            break;
        case CPUID_RTPS + 0:
        case CPUID_RTPS + 1:
            ASSERT(stage == NUM_STAGES - 1); // first stage is handled by the target CPU
            reset_component(COMPONENT_RTPS);
	    wdt_kick(wdt); // reset the counter (re-enabling alone does do that)
	    wdt_enable(wdt);
            break;
        case CPUID_HPPS + 0:
        case CPUID_HPPS + 1:
        case CPUID_HPPS + 2:
        case CPUID_HPPS + 3:
        case CPUID_HPPS + 4:
        case CPUID_HPPS + 5:
        case CPUID_HPPS + 6:
        case CPUID_HPPS + 7:
            ASSERT(stage == NUM_STAGES - 1); // first stage is handled by the target CPU
            reset_component(COMPONENT_HPPS);
            break;
        default:
                ASSERT(false && "invalid context");
    }
}

int watchdog_trch_start() {
    int rc = 1;
    trch_wdt = wdt_create_monitor("TRCH", WDT_TRCH_BASE,
                                  handle_timeout, (void *)CPUID_TRCH);

    if (!trch_wdt)
        return 1;

    uint64_t timeouts[] = { 5000000, 5000000 }; // about 2 sec each wall-clock in Qemu
    rc = wdt_configure(trch_wdt, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(trch_wdt);
	return rc;
    }
    
    wdt_enable(trch_wdt);
    nvic_int_enable(WDT_TRCH_ST1_IRQ);
    return 0;
}

void watchdog_trch_stop() {
    // NOTE: order: ISR might be called during destroy if IRQ is not disabled
    nvic_int_disable(WDT_TRCH_ST1_IRQ);
    wdt_disable(trch_wdt);
    wdt_destroy(trch_wdt);
}

void watchdog_trch_kick() {
    wdt_kick(trch_wdt);
}

int watchdog_rtps_start() {
    // TODO: start both WDTs (for both cores), and for now on RTPS side, have
    // Core 0 kick both, until we implement SPLIT mode

    uint64_t timeouts[] = { 10000000, 50000000 }; // about 1 sec, 5 sec
    rtps_wdt = wdt_create_monitor("RTPS0", WDT_RTPS0_TRCH_BASE,
                                  handle_timeout, (void *)(CPUID_RTPS + 0));
    if (!rtps_wdt)
        return 1;

    int rc = wdt_configure(rtps_wdt, NUM_STAGES, timeouts);
    if (rc) {
	wdt_destroy(rtps_wdt);
	return rc;
    }

    wdt_enable(rtps_wdt);
    nvic_int_enable(WDT_RTPS0_ST2_IRQ);
    return 0;
}

void watchdog_rtps_stop() {
    // NOTE: order: ISR might be called during destroy if IRQ is not disabled
    nvic_int_disable(WDT_RTPS0_ST2_IRQ);
    wdt_disable(rtps_wdt);
    wdt_destroy(rtps_wdt);
}

void wdt_rtps0_st2_isr()
{
    wdt_isr(rtps_wdt, /* stage (zero-based) */ 1);
}

void wdt_trch_st1_isr()
{
    wdt_isr(trch_wdt, /* stage */ 0);
}
void wdt_trch_st2_isr()
{
    wdt_isr(trch_wdt, /* stage */ 1);
}
