#define DEBUG 1

#include "object.h"
#include "panic.h"
#include "printf.h"
#include "regops.h"

#include "rti-timer.h"

#define REG__INTERVAL_LO       0x00
#define REG__INTERVAL_HI       0x04
#define REG__COUNT_LO           0x08
#define REG__COUNT_HI           0x0c

#define REG__CMD_ARM           0x10
#define REG__CMD_FIRE          0x14

enum cmd {
    CMD_CAPTURE = 0,
    CMD_LOAD,
    NUM_CMDS,
};

struct cmd_code {
    uint32_t arm;
    uint32_t fire;
};

// cmd -> (arm, fire)
static const struct cmd_code cmd_codes[] = {
    [CMD_CAPTURE] = { 0xCD01, 0x01CD },
    [CMD_LOAD]    = { 0xCD02, 0x02CD },
};

struct rti_timer {
    struct object obj;
    uintptr_t base;
    const char *name;
    rti_timer_cb_t *cb;
    void *cb_arg;
};

#define MAX_TIMERS 12
static struct rti_timer rti_timers[MAX_TIMERS];

static void exec_cmd(struct rti_timer *tmr, enum cmd cmd)
{
    // TODO: Arm-Fire is no protection from a stray jump over here...
    // Are we supposed to separate these with data-driven control flow?
    const struct cmd_code *code = &cmd_codes[cmd];
    REGB_WRITE32(tmr->base, REG__CMD_ARM,  code->arm);
    REGB_WRITE32(tmr->base, REG__CMD_FIRE, code->fire);
}

struct rti_timer *rti_timer_create(const char *name, uintptr_t base,
                                   rti_timer_cb_t *cb, void *cb_arg)
{
    printf("RTI TMR %s: create base %p\r\n", name, base);
    struct rti_timer *tmr = OBJECT_ALLOC(rti_timers);
    tmr->base = base;
    tmr->name = name;
    tmr->cb = cb;
    tmr->cb_arg = cb_arg;
    return tmr;
}

void rti_timer_destroy(struct rti_timer *tmr)
{
    ASSERT(tmr);
    printf("RTI TMR %s: destroy\r\n", tmr->name);
    OBJECT_FREE(tmr);
}

uint64_t rti_timer_capture(struct rti_timer *tmr)
{
    ASSERT(tmr);
    exec_cmd(tmr, CMD_CAPTURE);
    uint64_t count = ((uint64_t)REGB_READ32(tmr->base, REG__COUNT_HI) << 32) |
                     REGB_READ32(tmr->base, REG__COUNT_LO);
    DPRINTF("RTI TMR %s: count -> 0x%08x%08x\r\n", tmr->name,
            (uint32_t)(count >> 32), (uint32_t)count);
    return count;
}

void rti_timer_configure(struct rti_timer *tmr, uint64_t interval)
{
    ASSERT(tmr);
    REGB_WRITE32(tmr->base, REG__INTERVAL_HI, interval >> 32);
    REGB_WRITE32(tmr->base, REG__INTERVAL_LO, interval & 0xffffffff);
    DPRINTF("RTI TMR %s: interval <- 0x%08x%08x\r\n", tmr->name,
            (uint32_t)(interval >> 32), (uint32_t)interval);
    exec_cmd(tmr, CMD_LOAD);
}

void rti_timer_isr(struct rti_timer *tmr)
{
    ASSERT(tmr);
    DPRINTF("RTI TMR %s: ISR\r\n", tmr->name);

    if (tmr->cb)
        tmr->cb(tmr, tmr->cb_arg);

    // Interrupt is edge-triggered, and clears HW de-asserts it on next cycle
}
