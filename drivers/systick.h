#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#define SYSTICK_CLK_HZ 1000000 // appears to be fixed in Qemu model

typedef void (systick_cb_t)(void *arg);

void systick_config(uint32_t interval, systick_cb_t *cb, void *cb_arg);
void systick_clear();
uint32_t systick_count();
void systick_enable();
void systick_disable();

#endif // SYSTICK_H
