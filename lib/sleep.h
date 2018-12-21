#ifndef SLEEP_H
#define SLEEP_H

void mdelay(unsigned ms); // busyloop
void sleep_set_busyloop_factor(unsigned f);

#if TEST_SLEEP_TIMER
void sleep_set_clock(unsigned t);
void msleep(unsigned ms);

// Call this from a timer ISR
void sleep_tick(unsigned delta);
#else // !TEST_SLEEP_TIMER
#define msleep(t) mdelay(t)
#endif // !TEST_SLEEP_TIMER

#endif // SLEEP_H
