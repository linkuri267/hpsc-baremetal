#include <stdint.h>

#include "printf.h"
#include "hwinfo.h"
#include "gic.h"
#include "sleep.h"
#include "gtimer.h"

#define FREQ_HZ 1000000

#define TICKS_TO_TEST 3

#define INTERVAL_MS 2000
#define INTERVAL_CYCLES (INTERVAL_MS * (FREQ_HZ / 1000))

struct context {
    enum gtimer timer;
    unsigned ticks;
};

static void timer_tick(void *arg)
{
    struct context *ctx = (struct context *)arg;
    ++ctx->ticks;
    int32_t tval = gtimer_get_tval(ctx->timer); // negative value, time since last tick
    uint64_t pctval = gtimer_get_pct(ctx->timer);
    printf("TICK: timer %u pct 0x%08x%08x ticks %u tval %d\r\n",
           ctx->timer, (uint32_t)(pctval>>32), (uint32_t)(pctval & 0xffffff),
           ctx->ticks, tval);
    if (ctx->ticks < TICKS_TO_TEST)
        gtimer_set_tval(ctx->timer, INTERVAL_CYCLES); // schedule the next tick
    else
        gtimer_stop(ctx->timer);
}

static bool check_ticks(unsigned ticks, unsigned expected)
{
    if (ticks < expected) {
        printf("ERROR: TEST: gtimer: unexpected ticks: %u < %u\r\n",
                ticks, expected);
        return false;
    }
    printf("TEST: gtimer: correct ticks: %u >= %u\r\n", ticks, expected);
    return true;
}

int test_gtimer()
{
    int rc = 0;
    uint32_t frq = gtimer_get_frq();

#if 0 // only allowed from EL2
    gtimer_set_frq(FREQ_HZ);
    frq = gtimer_get_frq();
    if (frq != FREQ_HZ) {
        printf("ERROR: gtimer test: failed to set counter frq: %u != %u\r\n",
               frq, FREQ_HZ);
        return 1;
    }
#endif
    gic_int_enable(PPI_IRQ__TIMER_PHYS, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);
    gic_int_enable(PPI_IRQ__TIMER_VIRT, GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL);
    // gic_int_enable(PPI_IRQ__TIMER_HYP,  GIC_IRQ_TYPE_PPI, GIC_IRQ_CFG_LEVEL); // from EL2 only

    enum gtimer el1_timers[] = { GTIMER_PHYS, GTIMER_VIRT };

    for (int i = 0; i < sizeof(el1_timers) / sizeof(el1_timers[0]); ++i) {
        enum gtimer timer = el1_timers[i];

        uint64_t pctval_a = gtimer_get_pct(timer);
        uint32_t cntval_a = gtimer_get_tval(timer);
        printf("timer %u: pct 0x%08x%08x tval %u (0x%x)\r\n",
                timer, (uint32_t)(pctval_a>>32), (uint32_t)(pctval_a & 0xffffffff),
                cntval_a, cntval_a);

        uint64_t pctval_b = gtimer_get_pct(timer);
        uint32_t cntval_b = gtimer_get_tval(timer);
        printf("timer %u: pct 0x%08x%08x tval %u (0x%x)\r\n",
                timer, (uint32_t)(pctval_b>>32), (uint32_t)(pctval_b & 0xffffffff),
                cntval_b, cntval_b);

        if (pctval_a >= pctval_b) {
            printf("ERROR: timer %u: CNTPCT value did not advance\r\n", timer);
            return 1;
        }

        gtimer_set_tval(timer, INTERVAL_CYCLES); // schedule a tick

        struct context ctx = {
                .timer = timer,
                .ticks = 0
        };

        gtimer_subscribe(timer, timer_tick, &ctx);

        gtimer_start(timer);
        mdelay(INTERVAL_MS);
        if (!check_ticks(ctx.ticks, 1)) goto cleanup_running;
        mdelay(INTERVAL_MS);
        if (!check_ticks(ctx.ticks, 2)) goto cleanup_running;

        gtimer_stop(timer);
        mdelay(INTERVAL_MS);
        if (!check_ticks(ctx.ticks, 2)) goto cleanup;
        mdelay(INTERVAL_MS);
        if (!check_ticks(ctx.ticks, 2)) goto cleanup;

        printf("TEST gtimer %u: Ticked %u times\r\n", timer, ctx.ticks);
        gtimer_unsubscribe(timer);
        continue;

cleanup_running:
	gtimer_stop(timer);
cleanup:
        gtimer_unsubscribe(timer);
	rc = 1;
	break;
   }

    gic_int_disable(PPI_IRQ__TIMER_PHYS, GIC_IRQ_TYPE_PPI);
    gic_int_disable(PPI_IRQ__TIMER_VIRT, GIC_IRQ_TYPE_PPI);
    // gic_int_disable(PPI_IRQ__TIMER_HYP, GIC_IRQ_TYPE_PPI); // from EL2 only

    return rc;
}
