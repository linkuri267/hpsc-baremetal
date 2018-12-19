#ifndef SLEEP_H
#define SLEEP_H

void delay(unsigned iters);

#if TEST_SLEEP_TIMER
void sleep(unsigned t); // units defined by clock (see below)

// These define the unit of the time passed to sleep
void sleep_set_clock(unsigned t);
unsigned sleep_get_clock();

// Call this from a timer ISR
void sleep_tick(unsigned delta);
#else // !TEST_SLEEP_TIMER
#define sleep(t) delay(t)
#endif // !TEST_SLEEP_TIMER

#endif // SLEEP_H
