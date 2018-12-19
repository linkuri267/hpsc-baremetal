#include "printf.h"
#include "panic.h"

#include "sleep.h"

// A very very approximate conversion factor.
// You'd think it would be less than 1, but no (empirically determined).
#define CYCLES_TO_ITERS 25

#if TEST_SLEEP_TIMER
#define DEFAULT_CLK 0
#else // !TEST_SLEEP_TIMER (i.e. busyloop delay)
#define DEFAULT_CLK 1000000 // cycles to iterations (TODO measure)
#endif // !TEST_SLEEP_TIMER

static volatile unsigned time = 0;
static volatile unsigned clk = DEFAULT_CLK;

#define MAX_TIME ~0

void sleep_set_clock(unsigned t)
{
    clk = t;
}

unsigned sleep_get_clock() {
    return clk;
}

// Call this from a timer ISR
void sleep_tick(unsigned delta)
{
    time += delta;
    printf("SLEEP: time += %u -> %u\r\n", delta, time);
}
void sleep(unsigned t)
{
    ASSERT(clk && "sleep clk not set");

    unsigned wakeup = time + t; // % MAX_TIME implicitly
    printf("time %u, wakeup %u\r\n", time, wakeup);
    int remaining = wakeup >= time ? wakeup - time : (MAX_TIME - time) + wakeup;
    unsigned last_tick = time;
    while (remaining > 0) {
	printf("SLEEP: sleeping for %u...\r\n", remaining);
        asm("wfi"); // TODO: change to WFE and add SEV to sleep_tick

	unsigned elapsed = time >= last_tick ? time - last_tick : (MAX_TIME - last_tick) + time;
        printf("time %u, last_tick %u el %u\r\n", time, last_tick, elapsed);
        last_tick = time;
        remaining -= elapsed;
    }
    printf("SLEEP: awake\r\n");
}

void delay(unsigned t)
{
    volatile unsigned work = 0;
    for (int i = 0; i < t; ++i)
        work++;
}
