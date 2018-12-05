#include <stdint.h>

#include "object.h"
#include "regops.h"
#include "panic.h"
#include "printf.h"

#include "wdt.h"

// Stage-relative offsets
#define REG__TERMINAL        0x00
#define REG__COUNT           0x08

#define NUM_STAGES           2
#define STAGE_REG_SIZE       8

// Global offsets
#define REG__CONFIG          0x20
#define REG__STATUS          0x24

#define REG__CMD_ARM         0x28
#define REG__CMD_FIRE        0x2c

// Fields
#define REG__CONFIG__EN      0x1
#define REG__CONFIG__TICKDIV__SHIFT 2
#define REG__STATUS__TIMEOUT__SHIFT 0

#define HPSC_WDT_COUNTER_WIDTH (64 - (NUM_STAGES - 1)) // see comments in Qemu model

#define STAGE_REGS_SIZE (NUM_STAGES * STAGE_REG_SIZE) // register space size per stage
#define STAGE_REG(reg, stage) ((STAGE_REGS_SIZE * stage) + reg)

// The split by command type is for driver implementation convenience, in
// hardware, there is only one command type.

enum cmd {
    CMD_DISABLE = 0,
    NUM_CMDS,
};

enum stage_cmd {
    SCMD_CAPTURE = 0,
    SCMD_LOAD,
    SCMD_CLEAR,
    NUM_SCMDS,
};

struct cmd_code {
    uint32_t arm;
    uint32_t fire;
};

// Store the codes in RO memory section. From fault tolerance perspective,
// there is not a big difference with storing them in instruction memory (i.e.
// by using them as immediate values) -- and the compiler is actually free to
// do so. Both are protected by the MPU, and an upset in both will be detected
// by ECC. In both cases, the value can end up in the cache (either I or D,
// both of which have ECC). Also, in either case, the code will end up in a
// register (which is protected by TMR, in case of TRCH).

#define MAX_STAGES 2 // re-configurable by a macro in Qemu model (requires Qemu rebuild)

// stage -> cmd -> (arm, fire)
static const struct cmd_code stage_cmd_codes[][NUM_SCMDS] = {
    [0] = {
        [SCMD_CAPTURE] = { 0xCD01, 0x01CD },
        [SCMD_LOAD] =    { 0xCD03, 0x03CD },
        [SCMD_CLEAR] =   { 0xCD05, 0x05CD },
    },
    [1] = {
        [SCMD_CAPTURE] = { 0xCD02, 0x02CD },
        [SCMD_LOAD] =    { 0xCD04, 0x04CD },
        [SCMD_CLEAR] =   { 0xCD06, 0x06CD },
    },
};

// cmd -> (arm, fire)
static const struct cmd_code cmd_codes[] = {
    [CMD_DISABLE] = { 0xCD07, 0x07CD },
};

struct wdt {
    struct object obj;
    volatile uint32_t *base; 
    const char *name;
    wdt_cb_t cb;
    void *cb_arg;
    unsigned num_stages;
    bool monitor; // only the monitor can configure the timer
};

#define MAX_WDTS 16
static struct wdt wdts[MAX_WDTS];

static void exec_cmd(struct wdt *wdt, const struct cmd_code *code)
{
    // TODO: Arm-Fire is no protection from a stray jump over here...
    // Are we supposed to separate these with data-driven control flow?
    REGB_WRITE32(wdt->base, REG__CMD_ARM,  code->arm);
    REGB_WRITE32(wdt->base, REG__CMD_FIRE, code->fire);
}

static void exec_global_cmd(struct wdt *wdt, enum cmd cmd)
{
    printf("WDT %s: exec cmd: %u\r\n", wdt->name, cmd);
    ASSERT(cmd < NUM_CMDS);
    exec_cmd(wdt, &cmd_codes[cmd]);
}
static void exec_stage_cmd(struct wdt *wdt, enum stage_cmd scmd, unsigned stage)
{
    printf("WDT %s: stage %u: exec stage cmd: %u\r\n", wdt->name, stage, scmd);
    ASSERT(stage < MAX_STAGES);
    ASSERT(scmd < NUM_SCMDS);
    exec_cmd(wdt, &stage_cmd_codes[stage][scmd]);
}

static struct wdt *wdt_create(const char *name, volatile uint32_t *base,
                       wdt_cb_t cb, void *cb_arg)
{

    printf("WDT %s: create base %p\r\n", name, base);

    struct wdt *wdt = OBJECT_ALLOC(wdts);
    wdt->base = base;
    wdt->name = name;
    wdt->monitor = false;
    wdt->cb = cb;
    wdt->cb_arg = cb_arg;

    return wdt;
}
struct wdt *wdt_create_monitor(const char *name, volatile uint32_t *base,
                       wdt_cb_t cb, void *cb_arg)
{
    struct wdt *wdt = wdt_create(name, base, cb, cb_arg);
    wdt->monitor = true;
    return wdt;
}
struct wdt *wdt_create_target(const char *name, volatile uint32_t *base,
                       wdt_cb_t cb, void *cb_arg)
{
    struct wdt *wdt = wdt_create(name, base, cb, cb_arg);
    wdt->monitor = false;
    return wdt;
}

int wdt_configure(struct wdt *wdt, unsigned freq,
                  unsigned num_stages, uint64_t *timeouts)
{
    ASSERT(wdt);
    ASSERT(wdt->monitor);
    ASSERT(!wdt_is_enabled(wdt)); // not strict requirement, but for sanity
    if (num_stages > MAX_STAGES) {
        printf("ERROR: WDT: more stages than supported: %u >= %u\r\n",
               num_stages, MAX_STAGES);
        return 1;
    }
    if (!(WDT_MIN_FREQ_HZ <= freq && freq <= WDT_MAX_FREQ_HZ && freq % WDT_MIN_FREQ_HZ == 0)) {
        printf("ERROR: WDT: freq is out of range: %u not a multiple of %u in [%u, %u]\r\n",
               WDT_MIN_FREQ_HZ, freq, WDT_MIN_FREQ_HZ, WDT_MAX_FREQ_HZ);
        return 1;
    }
    for (unsigned stage = 0; stage < num_stages; ++stage) {
        if (timeouts[stage] & (~0ULL << HPSC_WDT_COUNTER_WIDTH)) {
                printf("ERROR: WDT: timeout for stage %u exceeds counter width (%u bits): %08x%08x\r\n",
                       stage, HPSC_WDT_COUNTER_WIDTH,
                      (uint32_t)(timeouts[stage] >> 32), (uint32_t)(timeouts[stage] & 0xffffffff));
                return 2;
        }
    }

    wdt->num_stages = num_stages;

    // Set divider and zero other fields
    ASSERT(freq % WDT_MIN_FREQ_HZ == 0);
    unsigned div = WDT_MAX_FREQ_HZ / freq;
    printf("WDT %s: set divider to %u\r\n", wdt->name, div);
    REGB_WRITE32(wdt->base, REG__CONFIG, div << REG__CONFIG__TICKDIV__SHIFT);

    for (unsigned stage = 0; stage < num_stages; ++stage) {
        // Loading alone does not clear the current count (which may have been
        // left over if timer previously enabled and disabled).
        exec_stage_cmd(wdt, SCMD_CLEAR, stage);

        REGB_WRITE64(wdt->base, STAGE_REG(REG__TERMINAL, stage), timeouts[stage]);
        exec_stage_cmd(wdt, SCMD_LOAD, stage);
    }
    return 0;
}

void wdt_destroy(struct wdt *wdt)
{
    ASSERT(wdt);
    printf("WDT %s: destroy\r\n", wdt->name);
    if (wdt->monitor)
        ASSERT(!wdt_is_enabled(wdt));
    OBJECT_FREE(wdt);
}

uint64_t wdt_count(struct wdt *wdt, unsigned stage)
{
    ASSERT(wdt);
    exec_stage_cmd(wdt, SCMD_CAPTURE, stage); 
    uint64_t count = REGB_READ64(wdt->base, STAGE_REG(REG__COUNT, stage));
    printf("WDT %s: count -> 0x%08x%08x\r\n", wdt->name,
           (uint32_t)(count >> 32), (uint32_t)(count & 0xffffffff));
    return count;
}

uint64_t wdt_timeout(struct wdt *wdt, unsigned stage)
{
    ASSERT(wdt);
    // NOTE: not going to be the right value if it wasn't not loaded via cmd
    uint64_t terminal = REGB_READ64(wdt->base, STAGE_REG(REG__TERMINAL, stage));
    printf("WDT %s: terminal -> 0x%08x%08x\r\n", wdt->name,
           (uint32_t)(terminal >> 32), (uint32_t)(terminal & 0xffffffff));
    return terminal;
}

bool wdt_is_enabled(struct wdt *wdt)
{
    bool enabled = REGB_READ32(wdt->base, REG__CONFIG) & REG__CONFIG__EN;
    printf("WDT %s: is enabled -> %u\r\n", wdt->name, enabled);
    return enabled;
}

void wdt_enable(struct wdt *wdt)
{
    ASSERT(wdt);
    printf("WDT %s: enable\r\n", wdt->name);
    REGB_SET32(wdt->base, REG__CONFIG, REG__CONFIG__EN);
}

void wdt_disable(struct wdt *wdt)
{
    ASSERT(wdt);
    ASSERT(wdt->monitor);
    printf("WDT %s: disable\r\n", wdt->name);
    exec_global_cmd(wdt, CMD_DISABLE);
}

void wdt_kick(struct wdt *wdt)
{
    ASSERT(wdt);
    printf("WDT %s: kick\r\n", wdt->name);
    // In Concept A variant, there is only a clear for stage 0.  In Concept B
    // variant, there's a clear for each stage, but it is suffient to clear the
    // first stage, because that action has to stop the timers for downstream
    // stages in HW, according to the current interpretation of the HW spec.
    exec_stage_cmd(wdt, SCMD_CLEAR, /* stage */ 0);
}

void wdt_isr(struct wdt *wdt, unsigned stage)
{
    ASSERT(wdt);
    printf("WDT %s: ISR\r\n", wdt->name);
    // TODO: spec unclear: if we are not allowed to clear the int source, then
    // we have to disable the interrupt via the interrupt controller, and
    // re-enable it in wdt_enable.
    REGB_CLEAR32(wdt->base, REG__STATUS, 1 << ( REG__STATUS__TIMEOUT__SHIFT + stage));

    if (wdt->cb)
        wdt->cb(wdt, stage, wdt->cb_arg);
}
