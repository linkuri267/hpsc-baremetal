#define DEBUG 1

#include <stdint.h>

#include "panic.h"
#include "object.h"
#include "regops.h"

#include "etimer.h"

#define REG__CAPTURED_LO       0x00
#define REG__CAPTURED_HI       0x04
#define REG__LOAD_LO           0x08
#define REG__LOAD_HI           0x0c
#define REG__EVENT_LO          0x10
#define REG__EVENT_HI          0x14
#define REG__SYNC_LO           0x18
#define REG__SYNC_HI           0x1c
#define REG__PAUSE             0x20
#define REG__SYNC_INTERVAL     0x24
#define REG__STATUS            0x28
#define REG__SYNC_PERIOD       0x2c

#define REG__CONFIG_LO         0x30
#define REG__CONFIG_HI         0x34
#define REG__PULSE_THRES       0x38
#define REG__PULSE_CONFIG      0x3c
#define REG__CMD_ARM           0x40
#define REG__CMD_FIRE          0x44

#define REG__STATUS__EVENT       0x1
#define REG__STATUS__SYNC        0x2
#define REG__STATUS__PAUSE       0x4
#define REG__STATUS__PULSE_OUT   0x8
#define REG__STATUS_SYNC_PERIOD 0x10

#define REG__CONFIG_LO__SYNC_SOURCE             0x7
#define REG__CONFIG_LO__SYNC_SOURCE__SHIFT        0
#define REG__CONFIG_LO__TICKDIV             0x7fff8
#define REG__CONFIG_LO__TICKDIV__SHIFT            3

#define REG__PULSE_CONFIG__PULSE_WIDTH 0xffff
#define REG__PULSE_CONFIG__PULSE_WIDTH__SHIFT 0

enum cmd {
    CMD_CAPTURE = 0,
    CMD_LOAD,
    CMD_EVENT,
    CMD_SYNC,
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
    [CMD_EVENT]   = { 0xCD03, 0x03CD },
    [CMD_SYNC]    = { 0xCD04, 0x04CD },
};

struct etimer {
    struct object obj;
    volatile uint32_t *base;
    const char *name;
    etimer_cb_t cb;
    void *cb_arg;
    uint32_t nominal_freq_hz;
    uint32_t clk_freq_hz;
    unsigned max_div;
    uint32_t sync_interval;
};

static struct etimer etimers[1];

static void exec_cmd(struct etimer *et, enum cmd cmd)
{
    // TODO: Arm-Fire is no protection from a stray jump over here...
    // Are we supposed to separate these with data-driven control flow?
    const struct cmd_code *code = &cmd_codes[cmd];
    REGB_WRITE32(et->base, REG__CMD_ARM,  code->arm);
    REGB_WRITE32(et->base, REG__CMD_FIRE, code->fire);
}

struct etimer *etimer_create(const char *name, volatile uint32_t *base,
                             etimer_cb_t cb, void *cb_arg,
                             uint32_t nominal_freq_hz, uint32_t clk_freq_hz,
                             unsigned max_div)
{
    printf("ETMR %s: create base %p\r\n", name, base);
    struct etimer *et = OBJECT_ALLOC(etimers);
    et->base = base;
    et->name = name;
    et->cb = cb;
    et->cb_arg = cb_arg;
    et->nominal_freq_hz = nominal_freq_hz;
    et->clk_freq_hz = clk_freq_hz;
    et->max_div = max_div;
    return et;
}

void etimer_destroy(struct etimer *et)
{
    ASSERT(et);
    printf("ETMR %s: destroy\r\n", et->name);
    OBJECT_FREE(et);
}

int etimer_configure(struct etimer *et, uint32_t freq,
                     enum etimer_sync_src sync_src,
                     uint32_t sync_interval)
{
    if (freq > et->clk_freq_hz || et->clk_freq_hz % freq != 0) {
        printf("ETMR %s: ERROR: freq (%u) not a divisor of clk freq (%u)\r\n",
               freq, et->clk_freq_hz);
        return 1;
    }
    unsigned tickdiv = et->clk_freq_hz / freq - 1;
    if (tickdiv > et->max_div) {
        printf("ETMR %s: ERROR: divider too large: %u > %u\r\n",
               tickdiv, et->max_div);
        return 1;
    }

    REGB_WRITE32(et->base, REG__CONFIG_LO,
            (tickdiv  << REG__CONFIG_LO__TICKDIV__SHIFT) |
            (sync_src << REG__CONFIG_LO__SYNC_SOURCE__SHIFT));

    et->sync_interval = sync_interval;
    REGB_WRITE32(et->base, REG__SYNC_INTERVAL, sync_interval);
    return 0;
}

uint64_t etimer_capture(struct etimer *et)
{
    exec_cmd(et, CMD_CAPTURE);
    uint64_t count = ((uint64_t)REGB_READ32(et->base, REG__CAPTURED_HI) << 32) |
                     REGB_READ32(et->base, REG__CAPTURED_LO);
    DPRINTF("ETMR %s: count -> 0x%08x%08x\r\n", et->name,
            (uint32_t)(count >> 32), (uint32_t)count);
    return count;
}

void etimer_load(struct etimer *et, uint64_t count)
{
    REGB_WRITE32(et->base, REG__LOAD_HI, count >> 32);
    REGB_WRITE32(et->base, REG__LOAD_LO, count & 0xffffffff);
    DPRINTF("ETMR %s: count <- 0x%08x%08x\r\n", et->name,
            (uint32_t)(count >> 32), (uint32_t)count);
    exec_cmd(et, CMD_LOAD);
}

void etimer_event(struct etimer *et, uint64_t count)
{
    REGB_WRITE32(et->base, REG__EVENT_HI, count >> 32);
    REGB_WRITE32(et->base, REG__EVENT_LO, count & 0xffffffff);
    DPRINTF("ETMR %s: event <- 0x%08x%08x\r\n", et->name,
            (uint32_t)(count >> 32), (uint32_t)count);
    exec_cmd(et, CMD_EVENT);
}

void etimer_pulse(struct etimer *et, uint32_t thres, uint16_t width)
{
    REGB_WRITE32(et->base, REG__PULSE_THRES, thres);
    REGB_WRITE32(et->base, REG__PULSE_CONFIG,
                 width << REG__PULSE_CONFIG__PULSE_WIDTH__SHIFT);
    DPRINTF("ETMR %s: pulse <- thres 0x%x width %u\r\n", thres, width);
    // TODO: spec: how to enable the output pulse generation?
}

uint64_t etimer_skew(struct etimer *et)
{
    // TODO: how is sync pause related to this? also sync period?

    uint64_t count_now = etimer_capture(et);

    // TODO: spec: how is this read going to be atomic? also it should be
    // atomic with capture. For now assuming that CMD_CAPTURE loads both
    // CAPTURE and SYNC_TIME registers.
    uint64_t count_sync = ((uint64_t)REGB_READ32(et->base, REG__SYNC_HI) << 32) |
                          REGB_READ32(et->base, REG__SYNC_LO);

    // If count_now/sync_interval < count_sync, then the skew overflowed,
    // and the unsigned cast of the negative skew is sensible.
    uint64_t skew = (count_now - ((uint32_t)count_now % et->sync_interval)) - count_sync;
    DPRINTF("ETMR %s: count @ sync -> 0x%08x%08x @ now -> 0x%08x%08x: skew 0x%08x%08x\r\n",
            et->name, (uint32_t)(count_sync >> 32), (uint32_t)count_sync,
            (uint32_t)(count_now >> 32), (uint32_t)count_now,
            (uint32_t)(skew >> 32), (uint32_t)skew);
    return skew;
}

void etimer_sync(struct etimer *et)
{
    DPRINTF("ETMR %s: sync\r\n", et->name);
    exec_cmd(et, CMD_SYNC);
}

void etimer_isr()
{
    struct etimer *et = &etimers[0]; // only one in the system
    DPRINTF("ETMR %s: ISR\r\n", et->name);

    if (et->cb)
        et->cb(et, et->cb_arg);

    REGB_SET32(et->base, REG__STATUS, REG__STATUS__EVENT); // W1C
}
