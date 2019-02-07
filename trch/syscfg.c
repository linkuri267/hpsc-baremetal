#include <stdint.h>

#include "printf.h"
#include "subsys.h"
#include "smc.h"
#include "syscfg.h"

static const char *bin_loc_name(unsigned m)
{
    switch (m) {
        case SYSCFG__BIN_LOC__SRAM: return "SRAM";
        case SYSCFG__BIN_LOC__DRAM: return "DRAM";
        default:                   return "?";
    };
}

static const char *rtps_mode_name(unsigned m)
{
    switch (m) {
        case SYSCFG__RTPS_MODE__SPLIT:    return "SPLIT";
        case SYSCFG__RTPS_MODE__LOCKSTEP: return "LOCKSTEP";
        case SYSCFG__RTPS_MODE__SMP:      return "SMP";
        default:                       return "?";
    };
}

void syscfg_print(struct syscfg *cfg)
{
    printf("SYSTEM CONFIG:\r\n"
           "\tbin loc:\t%s\r\n"
           "\tubsystems:\t%s\r\n"
           "\trtps mode:\t%s\r\n",
           bin_loc_name(cfg->bin_loc),
           subsys_name(cfg->subsystems),
           rtps_mode_name(cfg->rtps_mode));
}

int syscfg_load(struct syscfg *cfg)
{
    uint32_t *addr;
    uint32_t opts;

    if (smc_sram_load("syscfg", &addr))
        return 1;

    opts = *addr;
    printf("SYSCFG: raw: %x\r\n", opts);

    cfg->bin_loc = (opts & SYSCFG__BIN_LOC__MASK) >> SYSCFG__BIN_LOC__SHIFT;
    cfg->rtps_mode = (opts & SYSCFG__RTPS_MODE__MASK) >> SYSCFG__RTPS_MODE__SHIFT;
    cfg->subsystems = (opts & SYSCFG__SUBSYS__MASK) >> SYSCFG__SUBSYS__SHIFT;

    syscfg_print(cfg);
    return 0;
}
