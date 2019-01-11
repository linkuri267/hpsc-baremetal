#ifndef TEST_RTI_TIMER_H
#define TEST_RTI_TIMER_H

#include <stdint.h>
#include "rti-timer.h"

int test_rti_timer(volatile uint32_t *base, struct rti_timer **tmr_ptr);

#endif // TEST_RTI_TIMER_H
