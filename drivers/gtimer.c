#define DEBUG 0

#include <stdlib.h>
#include <stdint.h>

#include "console.h"
#include "panic.h"

#include "gtimer.h"

#define REG_CTRL__ENABLE		(1 << 0)
#define REG_CTRL__IT_MASK		(1 << 1)
#define REG_CTRL__IT_STAT		(1 << 2)

struct gtimer_state {
    gtimer_cb_t cb;
    void *cb_arg;
};

static struct gtimer_state timers[GTIMER_NUM_TIMERS] = {0};

static void ctrl_write(enum gtimer timer, uint32_t val)
{
    switch (timer) {
        case GTIMER_HYP:
            asm volatile("mcr p15, 4, %0, c14, c2, 1" : : "r" (val));
            break;
        case GTIMER_PHYS:
            asm volatile("mcr p15, 0, %0, c14, c2, 1" : : "r" (val));
            break;
        case GTIMER_VIRT:
            asm volatile("mcr p15, 0, %0, c14, c3, 1" : : "r" (val));
            break;
    }
}

static uint32_t ctrl_read(enum gtimer timer)
{
    uint32_t val;
    switch (timer) {
        case GTIMER_HYP:
            asm volatile("mrc p15, 4, %0, c14, c2, 1" : "=r" (val));
            break;
        case GTIMER_PHYS:
            asm volatile("mrc p15, 0, %0, c14, c2, 1" : "=r" (val));
            break;
        case GTIMER_VIRT:
            asm volatile("mrc p15, 0, %0, c14, c3, 1" : "=r" (val));
            break;
    }
    return val;
}

void gtimer_subscribe(enum gtimer timer, gtimer_cb_t cb, void *cb_arg)
{
    printf("GTIMER %u: sub\r\n", timer);

    timers[timer].cb = cb;
    timers[timer].cb_arg = cb_arg;
}

void gtimer_unsubscribe(enum gtimer timer)
{
    printf("GTIMER %u: unsub\r\n", timer);

    timers[timer].cb = NULL;
    timers[timer].cb_arg = NULL;
}

uint32_t gtimer_get_frq()
{
    uint32_t frq;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (frq));
    DPRINTF("GTIMER: frq -> %u\r\n", frq);
    return frq;
}

// Will only work from EL2
void gtimer_set_frq(uint32_t frq)
{
    printf("GTIMER: frq <- %u\r\n", frq);
    asm volatile("mcr p15, 0, %0, c14, c0, 0" : : "r" (frq));
}

uint64_t gtimer_get_pct(enum gtimer timer)
{
    uint32_t valhi, vallo;
    asm volatile("isb"); // see G5.2.1 in ARMv8 Reference
    switch (timer) {
        case GTIMER_HYP: // maps to PHYS
        case GTIMER_PHYS:
            asm volatile("mrrc p15, 0, %0, %1, c14" : "=r" (vallo), "=r" (valhi));
            break;
        case GTIMER_VIRT:
            asm volatile("mrrc p15, 1, %0, %1, c14" : "=r" (vallo), "=r" (valhi));
            break;
    }
    uint64_t val = ((uint64_t)valhi << 32) | vallo;
    DPRINTF("GTIMER %u: pct -> 0x%08x%08x\r\n", timer,
           (uint32_t)(val>>32), (uint32_t)(val & 0xffffffff));
    return val;
}

int32_t gtimer_get_tval(enum gtimer timer)
{
    int32_t val;
    asm volatile("isb"); // see G5.2.1 in ARMv8 Reference
    switch (timer) {
        case GTIMER_PHYS:
            asm volatile("mrc p15, 0, %0, c14, c2, 0" : "=r" (val));
            break;
        case GTIMER_VIRT:
            asm volatile("mrc p15, 0, %0, c14, c3, 0" : "=r" (val));
            break;
        case GTIMER_HYP:
            asm volatile("mrc p15, 4, %0, c14, c2, 0" : "=r" (val));
            break;
    }
    DPRINTF("GTIMER %u: tval -> %d (0x%x)\r\n", timer, val);
    return val;
}

void gtimer_set_tval(enum gtimer timer, int32_t val)
{
    DPRINTF("GTIMER %u: tval <- %d\r\n", timer, val);
    switch (timer) {
        case GTIMER_PHYS:
            asm volatile("mcr p15, 0, %0, c14, c2, 0" : : "r" (val));
            break;
        case GTIMER_VIRT:
            asm volatile("mcr p15, 0, %0, c14, c3, 0" : : "r" (val));
            break;
        case GTIMER_HYP:
            asm volatile("mcr p15, 4, %0, c14, c2, 0" : : "r" (val));
            break;
    }
}

uint64_t gtimer_get_cval(enum gtimer timer)
{
    uint32_t vallo, valhi;
    asm volatile("isb"); // see G5.2.1 in ARMv8 Reference
    switch (timer) {
        case GTIMER_HYP:
            asm volatile("mrrc p15, 6, %0, %1, c14" : "=r" (vallo), "=r" (valhi));
            break;
        case GTIMER_PHYS:
            asm volatile("mrrc p15, 2, %0, %1, c14" : "=r" (vallo), "=r" (valhi));
            break;
        case GTIMER_VIRT:
            asm volatile("mrrc p15, 3, %0, %1, c14" : "=r" (vallo), "=r" (valhi));
            break;
    }
    uint64_t val = ((uint64_t)valhi << 32) | vallo;
    DPRINTF("GTIMER %u: cval -> %08x%08x\r\n", timer, valhi, vallo);
    return val;
}

void gtimer_set_cval(enum gtimer timer, uint64_t val)
{
    uint32_t valhi = val >> 32;
    uint32_t vallo = val & 0xffffffff;
    DPRINTF("GTIMER %u: cval <- %08x%08x\r\n", timer, valhi, vallo);
    switch (timer) {
        case GTIMER_PHYS:
            asm volatile("mcrr p15, 2, %0, %1, c14" : : "r" (vallo), "r" (valhi));
            break;
        case GTIMER_VIRT:
            asm volatile("mcrr p15, 3, %0, %1, c14" : : "r" (vallo), "r" (valhi));
            break;
        case GTIMER_HYP:
            asm volatile("mcrr p15, 6, %0, %1, c14" : : "r" (vallo), "r" (valhi));
            break;
    }
}

void gtimer_start(enum gtimer timer)
{
    unsigned long ctrl = ctrl_read(timer);
    ctrl |= REG_CTRL__ENABLE;
    printf("GTIMER %u: start: CTRL <- %x\r\n", timer, ctrl);
    ctrl_write(timer, ctrl);
}

void gtimer_stop(enum gtimer timer)
{
    unsigned long ctrl = ctrl_read(timer);
    ctrl &= ~REG_CTRL__ENABLE;
    printf("GTIMER %u: stop: CTRL <- %x\r\n", timer, ctrl);
    ctrl_write(timer, ctrl);
}

void gtimer_isr(enum gtimer timer)
{
    uint32_t ctrl = ctrl_read(timer);
    DPRINTF("GTIMER %u: isr: ctlr %x\r\n", timer, ctrl);

    // shouldn't get the ISR, but this check exists in Linux
    if (!(ctrl & REG_CTRL__IT_STAT))
        return;

    ctrl |= REG_CTRL__IT_MASK;
    ctrl_write(timer, ctrl);

    struct gtimer_state *gtimer = &timers[timer];
    if (gtimer->cb)
        gtimer->cb(gtimer->cb_arg);

    ctrl = ctrl_read(timer); // must-reread in case modified in cb
    ctrl &= ~REG_CTRL__IT_MASK;
    ctrl_write(timer, ctrl);
}
