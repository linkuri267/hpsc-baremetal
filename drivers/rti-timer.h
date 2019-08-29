#ifndef RTI_TIMER_H
#define RTI_TIMER_H

#include <stdint.h>

struct rti_timer;

typedef void (rti_timer_cb_t)(struct rti_timer *tmr, void *arg);

struct rti_timer *rti_timer_create(const char *name, uintptr_t base,
                                   rti_timer_cb_t *cb, void *cb_arg);
void rti_timer_destroy(struct rti_timer *tmr);

uint64_t rti_timer_capture(struct rti_timer *tmr);
void rti_timer_configure(struct rti_timer *tmr, uint64_t interval);
void rti_timer_isr(struct rti_timer *tmr);

#endif // RTI_TIMER_H
