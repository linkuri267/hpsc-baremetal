#include <stdbool.h>

#include "systick.h"
#include "printf.h"
#include "delay.h"

#include "test.h"

#define INTERVAL 2000000 // @ ~1Mhz = 2s

#define INTERVAL_CPU_CYCLES (INTERVAL * 2) // conv factor is by trial and error

static void systick_tick(void *arg)
{
    unsigned *ticks = (unsigned *)arg;
    printf("TEST: systick: tick %u\r\n", *ticks);
    (*ticks)++;
}

static bool check_ticks(unsigned ticks, unsigned expected)
{
    if (ticks < expected) {
        printf("ERROR: TEST: systick: unexpected ticks: %u < %u\r\n",
                ticks, expected);
        return false;
    }
    printf("TEST: systick: correct ticks: %u >= %u\r\n", ticks, expected);
    return true;
}

int test_systick()
{
    int rc = 1;
    unsigned ticks = 0;
    systick_config(INTERVAL, /* callback */ systick_tick, &ticks);

    systick_enable();
    delay(INTERVAL_CPU_CYCLES);
    if (!check_ticks(ticks, 1)) goto cleanup;
    delay(INTERVAL_CPU_CYCLES);
    if (!check_ticks(ticks, 2)) goto cleanup;

    systick_disable();
    delay(INTERVAL_CPU_CYCLES);
    if (!check_ticks(ticks, 2)) goto cleanup;
    delay(INTERVAL_CPU_CYCLES);
    if (!check_ticks(ticks, 2)) goto cleanup;

    return 0;
cleanup:
    systick_disable();
    return rc;
}
