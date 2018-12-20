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
    // WARNING: don't add any printf statements into this functions, they
    // interfere with the timing, despite being outside of the busyloop itself.

    // treat iters as "cycles", use clock "frequency" to customize per CPU
    unsigned iters = ms * (clk / 1000);

    if (iters == 0)
        return;

    // Inline assembly, to avoid non-determinism (or total brakage) by
    // compiler optimizations. This asm is portable across armv7 and
    // aarch32, but won't necessary take the same time, so each CPU will
    // need its own calibration factor anyway (see above).
    asm (
     "delayloop:\n"
     "  nop\n"
     "  sub %0, #1\n"
     "  cmp %0, #0\n"
     "  bne delayloop\n"
     : : "r" (iters) :);
}
