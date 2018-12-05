#ifndef WDT_H
#define WDT_H

#include <stdint.h>
#include <stdbool.h>

/* WDT freq must be a multiple of min freq */
#define WDT_MIN_FREQ_HZ    3906250
#define WDT_MAX_FREQ_HZ 1000000000

struct wdt;

typedef void (*wdt_cb_t)(struct wdt *wdt, unsigned stage, void *arg);

// If fewer stages than the timer HW supports are passed here,
// then the interrupts of latter stages are ignored (but they
// still happen, since all stages are always active in HW).
struct wdt *wdt_create_monitor(const char *name, volatile uint32_t *base,
                               wdt_cb_t cb, void *cb_arg);
struct wdt *wdt_create_target(const char *name, volatile uint32_t *base,
                              wdt_cb_t cb, void *cb_arg);
void wdt_destroy(struct wdt *wdt);
int wdt_configure(struct wdt *wdt, unsigned freq,
                  unsigned num_stages, uint64_t *timeouts);
uint64_t wdt_count(struct wdt *wdt, unsigned stage);
uint64_t wdt_timeout(struct wdt *wdt, unsigned stage);
bool wdt_is_enabled(struct wdt *wdt);
void wdt_enable(struct wdt *wdt);
void wdt_disable(struct wdt *wdt);
void wdt_kick(struct wdt *wdt);

void wdt_isr(struct wdt *wdt, unsigned stage);

#endif // WDT_H
