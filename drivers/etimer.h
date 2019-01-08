#ifndef ETIMER_H
#define ETIMER_H

#include <stdint.h>

// The Elapsed Timer does not increment by one on each tick of its clock
// source. Instead, it increments by a delta given by: "nominal" tick frequency
// divided by the clock freq. This way the count can be interpreted as
// nanoseconds, regardless of the clock frequency.

struct etimer;

enum etimer_sync_src {
    ETIMER_SYNC_SW = 0,
    // other sync sources not implemented
};

typedef void (*etimer_cb_t)(struct etimer *et, void *arg);


struct etimer *etimer_create(const char *name, volatile uint32_t *base,
                             etimer_cb_t cb, void *cb_arg,
                             uint32_t nominal_freq_hz, uint32_t clk_freq_hz,
                             unsigned max_div);
void etimer_destroy(struct etimer *et);
int etimer_configure(struct etimer *et, uint32_t freq,
                     enum etimer_sync_src sync_source,
                     uint32_t sync_interval);
uint64_t etimer_capture(struct etimer *et);
void etimer_load(struct etimer *et, uint64_t count);
void etimer_event(struct etimer *et, uint64_t count);
void etimer_pulse(struct etimer *et, uint32_t thres, uint16_t width);
uint64_t etimer_skew(struct etimer *et); // at last sync event
void etimer_sync(struct etimer *et);

#endif // ETIMER_H
