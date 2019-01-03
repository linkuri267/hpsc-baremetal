#ifndef SLEEP_H
#define SLEEP_H

void mdelay(unsigned ms); // busyloop
void sleep_set_busyloop_factor(unsigned f);

#if CONFIG_SLEEP_TIMER
void sleep_set_clock(unsigned t);
void msleep(unsigned ms);

// Call this from a timer ISR
void sleep_tick(unsigned delta);
#else // !CONFIG_SLEEP_TIMER
#define msleep(t) mdelay(t)
#endif // !CONFIG_SLEEP_TIMER

#endif // SLEEP_H
