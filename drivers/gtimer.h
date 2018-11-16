#ifndef GTIMER_H
#define GTIMER_H

#include <stdint.h>
#include <stdbool.h>

enum gtimer {
        GTIMER_PHYS = 0,
        GTIMER_VIRT,
        GTIMER_HYP,
        /* GTIMER_SEC, */ /* physical secure, not implemented */
        GTIMER_NUM_TIMERS
};

// Returns whether to unmask the interrupt (i.e. keep getting ticks)
typedef void (*gtimer_cb_t)(void *);

void gtimer_subscribe(enum gtimer timer, gtimer_cb_t cb, void *cb_arg);
void gtimer_unsubscribe(enum gtimer timer);

uint32_t gtimer_get_frq();
void gtimer_set_frq(uint32_t frq);

uint64_t gtimer_get_pct(enum gtimer timer);
int32_t gtimer_get_tval(enum gtimer timer);
void gtimer_set_tval(enum gtimer timer, int32_t val);
uint64_t gtimer_get_cval(enum gtimer timer);
void gtimer_set_cval(enum gtimer timer, uint64_t val);

void gtimer_start(enum gtimer timer);
void gtimer_stop(enum gtimer timer);

void gtimer_isr(enum gtimer timer);

#endif // GTIMER_H
