#include "printf.h"
#include "panic.h"
#include "gic.h"
#include "hwinfo.h"
#include "wdt.h"

#include "watchdog.h"

struct wdt *wdt; // must be in global scope since needed by global ISR

// TODO: watchdogs for both cores
//struct wdt *wdts[NUM_CORES]; // must be in global scope since needed by global ISR

static void handle_timeout(struct wdt *wdt, unsigned stage, void *arg)
{
    printf("watchdog: expired\r\n");
    // nothing to do: main loop will return from WFI/WFE and kick
}

int watchdog_init()
{
    volatile unsigned expired_stage = 0;
    wdt = wdt_create_target("RTPS0", WDT_RTPS_RTPS_BASE + 0 * WDT_RTPS_SIZE,
                            handle_timeout, (void *)&expired_stage);
    if (!wdt)
        return 1;

    if (!wdt_is_enabled(wdt)) {
        printf("ERROR: wdt test: TRCH did not setup the WDT\r\n");
	wdt_destroy(wdt);
        return 1;
    }

    gic_int_enable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);
    return 0;
}

void watchdog_deinit()
{
    ASSERT(wdt);
    // NOTE: order: ISR might be called during destroy if IRQ is not disabled
    gic_int_disable(WDT_PPI_IRQ, GIC_IRQ_TYPE_PPI);
    wdt_destroy(wdt);
    wdt = NULL;
}

void watchdog_kick()
{
    ASSERT(wdt);
    wdt_kick(wdt);
}
