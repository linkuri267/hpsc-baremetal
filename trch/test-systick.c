#include <stdbool.h>

#include "systick.h"
#include "printf.h"
#include "sleep.h"

#include "test.h"

#define INTERVAL_MS 2000

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

    systick_config(INTERVAL_MS * (SYSTICK_CLK_HZ / 1000), systick_tick, &ticks);

    systick_enable();
    mdelay(INTERVAL_MS);
    if (!check_ticks(ticks, 1)) goto cleanup;
    mdelay(INTERVAL_MS);
    if (!check_ticks(ticks, 2)) goto cleanup;

    systick_disable();
    mdelay(INTERVAL_MS);
    if (!check_ticks(ticks, 2)) goto cleanup;
    mdelay(INTERVAL_MS);
    if (!check_ticks(ticks, 2)) goto cleanup;

    return 0;
cleanup:
    systick_disable();
    return rc;
}
