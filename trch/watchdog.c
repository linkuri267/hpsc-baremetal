#include <stdint.h>

#include "wdt.h"
#include "nvic.h"
#include "panic.h"
#include "reset.h"
#include "hwinfo.h"

#include "watchdog.h"

// Must be global for the standalone tests for WDTs in test-wdt.c
struct wdt *trch_wdt = NULL;
struct wdt *rtps_wdts[RTPS_NUM_CORES] = {0};
struct wdt *hpps_wdts[HPPS_NUM_CORES] = {0};

#define NUM_STAGES 2 // fixed in HW, but flexible in driver

// IDs for CPU 0 in each subsystem (private to this file)
#define CPUID_TRCH 0
#define CPUID_RTPS 1
#define CPUID_HPPS 3

static void handle_timeout(struct wdt *wdt, unsigned stage, void *arg)
{
    unsigned cpuid = (unsigned)arg;
    printf("watchdog: cpu %u: stage %u: expired\r\n", cpuid, stage);

    // Expand this with appropriate responses to faults
    switch (cpuid) {
        case CPUID_TRCH:
            ASSERT(stage == 0); // no last stage interrupt, because wired to hw reset
	    // nothing to do: main loop will return from WFI/WFE and kick
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
	    wdt_kick(wdt); // reset the counter (re-enabling alone does do that)
	    wdt_enable(wdt);
            break;
        default:
	    ASSERT(false && "invalid context");
    }
}

static struct wdt *create_wdt(const char *name, volatile uint32_t *base,
			      unsigned irq, uint64_t *timeouts, unsigned cpuid)
{
    struct wdt *wdt = wdt_create_monitor(name, base,
				         handle_timeout, (void *)cpuid);
    if (!wdt)
        return NULL;

    int rc = wdt_configure(wdt, NUM_STAGES, timeouts);
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
    uint64_t timeouts[] = { 5000000, 5000000 }; // about 2 sec each wall-clock in Qemu
    trch_wdt = create_wdt("TRCH", WDT_TRCH_BASE, WDT_TRCH_ST1_IRQ,
		      timeouts, CPUID_TRCH);
    if (!trch_wdt)
	return 1;
    return 0;
}

void watchdog_trch_stop() {
    destroy_wdt(trch_wdt, WDT_TRCH_ST1_IRQ);
    trch_wdt = NULL;
}

void watchdog_trch_kick() {
    wdt_kick(trch_wdt);
}

int watchdog_rtps_start() {
    int i;
    uint64_t timeouts[] = { 10000000, 50000000 }; // about 1 sec, 5 sec

    static const char * const rtps_wdt_names[HPPS_NUM_CORES] = {
	"RTPS0", "RTPS1"
    };

    for (i = 0; i < /* RTPS_NUM_CORES */ 1; ++i) { // TODO: start both timers in SPLIT mode
	ASSERT(i < sizeof(rtps_wdt_names) / sizeof(rtps_wdt_names[0]));
	rtps_wdts[i] = create_wdt(rtps_wdt_names[i],
		(volatile uint32_t *)((volatile uint8_t *)WDT_RTPS_TRCH_BASE +
						          i * WDT_RTPS_SIZE),
		WDT_RTPS_ST2_IRQ_START + i, timeouts, CPUID_RTPS + i);
	if (!rtps_wdts[i])
	    goto cleanup;
    }
    return 0;

cleanup:
    for (--i; i >= 0; --i) {
	destroy_wdt(rtps_wdts[i], WDT_RTPS_ST2_IRQ_START + i);
	rtps_wdts[i] = NULL;
    }
    return 1;
}

void watchdog_rtps_stop() {
    for (int i = 0; i < RTPS_NUM_CORES;  ++i) {
	destroy_wdt(rtps_wdts[i], WDT_RTPS_ST2_IRQ_START + i);
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
		WDT_HPPS_ST2_IRQ_START + i, timeouts, CPUID_HPPS + i);
	if (!hpps_wdts[i])
	    goto cleanup;
    }
    return 0;

cleanup:
    for (--i; i >= 0; --i) {
	destroy_wdt(hpps_wdts[i], WDT_HPPS_ST2_IRQ_START + i);
	hpps_wdts[i] = NULL;
    }
    return 1;
}

void watchdog_hpps_stop() {
    for (int i = 0; i < HPPS_NUM_CORES;  ++i) {
	destroy_wdt(hpps_wdts[i], WDT_HPPS_ST2_IRQ_START + i);
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
