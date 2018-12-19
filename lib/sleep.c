#include "printf.h"
#include "panic.h"

#include "sleep.h"

static volatile unsigned time = 0; // cycles @ clk Hz
static volatile unsigned clk = 0;

#define MAX_TIME ~0

void sleep_set_clock(unsigned f)
{
    clk = f;
}

#if TEST_SLEEP_TIMER
// Call this from a timer ISR
void sleep_tick(unsigned delta_cycles)
{
    time += delta_cycles;
    printf("SLEEP: time += %u -> %u\r\n", delta_cycles, time);
}
void msleep(unsigned ms)
{
    ASSERT(clk && "sleep clk not set");

    unsigned c = ms * (clk / 1000); // msec to cycles

    unsigned wakeup = time + c; // % MAX_TIME implicitly
    printf("time %u, wakeup %u\r\n", time, wakeup);
    int remaining = wakeup >= time ? wakeup - time : (MAX_TIME - time) + wakeup;
    unsigned last_tick = time;
    while (remaining > 0) {
	printf("SLEEP: sleeping for %u cycles...\r\n", remaining);
        asm("wfi"); // TODO: change to WFE and add SEV to sleep_tick

	unsigned elapsed = time >= last_tick ? time - last_tick : (MAX_TIME - last_tick) + time;
        last_tick = time;
        remaining -= elapsed;
    }
    printf("SLEEP: awake\r\n");
}
#endif // TEST_SLEEP_TIMER

void mdelay(unsigned ms)
{
#define MS_TO_ITERS 150 // empirical (and depends on load on virtual CPUs)
    volatile unsigned work = 0;
    for (int i = 0; i < ms * MS_TO_ITERS; ++i)
        work++;
}
