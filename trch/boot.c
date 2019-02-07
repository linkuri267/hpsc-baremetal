#include "printf.h"
#include "reset.h"
#include "mem-map.h"
#include "smc.h"
#include "watchdog.h"

#include "boot.h"

// Fields in the 32-bit value specifying the boot config
#define OPT__BIN_LOC__SHIFT             0
#define OPT__BIN_LOC__MASK              (0x1 << OPT__BIN_LOC__SHIFT)
#define OPT__RTPS_MODE__SHIFT           1
#define OPT__RTPS_MODE__MASK            (0x3 << OPT__RTPS_MODE__SHIFT)
#define OPT__SUBSYS__SHIFT              3
#define OPT__SUBSYS__MASK               (0xf << OPT__SUBSYS__SHIFT)

static subsys_t reboot_requests;

struct config {
    enum {
        CFG__BIN_LOC__SRAM = 0x1, // load HPPS/RTPS binaries from SRAM
        CFG__BIN_LOC__DRAM = 0x2, // assume HPPS/RTPS binaries already in DRAM
    } bin_loc;
    subsys_t subsystems; // bitmask of subsystems to boot
    enum {
        CFG__RTPS_MODE__SPLIT = 0x0,
        CFG__RTPS_MODE__LOCKSTEP = 0x1,
        CFG__RTPS_MODE__SMP = 0x2,
    } rtps_mode;
};

static struct config cfg;

static const char *bin_loc_name(unsigned m)
{
    switch (m) {
        case CFG__BIN_LOC__SRAM: return "SRAM";
        case CFG__BIN_LOC__DRAM: return "DRAM";
        default:                   return "?";
    };
}

static const char *rtps_mode_name(unsigned m)
{
    switch (m) {
        case CFG__RTPS_MODE__SPLIT:    return "SPLIT";
        case CFG__RTPS_MODE__LOCKSTEP: return "LOCKSTEP";
        case CFG__RTPS_MODE__SMP:      return "SMP";
        default:                       return "?";
    };
}

static void print_boot_cfg(struct config *bcfg)
{
    printf("BOOT: cfg:\r\n"
           "\tbin loc:\t%s\r\n"
           "\tubsystems:\t%s\r\n"
           "\trtps mode:\t%s\r\n",
           bin_loc_name(bcfg->bin_loc),
           subsys_name(bcfg->subsystems),
           rtps_mode_name(bcfg->rtps_mode));
}

static int boot_load(subsys_t subsys)
{
    if (cfg.bin_loc == CFG__BIN_LOC__DRAM) {
        printf("BOOT: nothing to load, according to boot cfg\r\n");
        return 0;
    }

    if (cfg.bin_loc != CFG__BIN_LOC__SRAM) {
        printf("BOOT: don't know how to load: invalid boot cfg\r\n");
        return 1;
    }

    switch (subsys) {
        case SUBSYS_RTPS_R52:
            printf("BOOT: load RTPS mode %s\r\n", rtps_mode_name(cfg.rtps_mode));
            switch (cfg.rtps_mode) {
                case CFG__RTPS_MODE__SPLIT: // TODO
                    printf("TODO: NOT IMPLEMENTED: loading for SPLIT mode");
                    break;
                case CFG__RTPS_MODE__LOCKSTEP:
                    if (smc_sram_load("rtps-bl"))
                        return 1;
                    if (smc_sram_load("rtps-os"))
                        return 1;
                    break;
                case CFG__RTPS_MODE__SMP: // TODO
                    printf("TODO: NOT IMPLEMENTED: loading for SMP mode");
                    break;
            }
            break;
        case SUBSYS_RTPS_A53:
            // TODO: load binaries
            break;
        case SUBSYS_HPPS:
            printf("BOOT: load HPPS\r\n");
            if (smc_sram_load("hpps-fw"))
                return 1;
            if (smc_sram_load("hpps-bl"))
                return 1;
            if (smc_sram_load("hpps-dt"))
                return 1;
            if (smc_sram_load("hpps-os"))
                return 1;
            break;
        default:
            printf("BOOT: ERROR: unknown subsystem %x\r\n", subsys);
            return 1;
     };
    return 0;
}

static int boot_reset(subsys_t subsys)
{
    int rc = 0;
    switch (subsys) {
        case SUBSYS_RTPS_R52:
            switch (cfg.rtps_mode) {
                case CFG__RTPS_MODE__SPLIT:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52_0);
                    watchdog_init_group(CPU_GROUP_RTPS_R52_1);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__SPLIT);
                    rc |= reset_release(COMP_CPUS_RTPS_R52);
                    break;
                case CFG__RTPS_MODE__LOCKSTEP:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52_0);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__LOCKSTEP);
                    rc = reset_release(COMP_CPU_RTPS_R52_0);
                    break;
                case CFG__RTPS_MODE__SMP:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__SPLIT);
                    rc = reset_release(COMP_CPU_RTPS_R52_0);
                    break;
                default:
                    printf("BOOT: ERROR: unknown RTPS boot mode: %s\r\n",
                           rtps_mode_name(cfg.rtps_mode));
                    return 1;
            }
            break;
        case SUBSYS_RTPS_A53:
#if CONFIG_RTPS_A53_WDT
            watchdog_init_group(CPU_GROUP_RTPS_A53);
#endif // CONFIG_RTPS_A53_WDT
            rc = reset_release(COMP_CPUS_RTPS_A53);
            break;
        case SUBSYS_HPPS:
#if CONFIG_HPPS_WDT
            watchdog_init_group(CPU_GROUP_HPPS);
#endif // CONFIG_HPPS_WDT
            rc = reset_release(COMP_CPU_HPPS_0);
            break;
        default:
            printf("BOOT: ERROR: invalid subsystem: %u\r\n", subsys);
            rc = 1;
    }
    return rc;
}

int boot_config()
{
    uint32_t opts = *(uint32_t *)TRCH_BOOT_CFG_ADDR;

    cfg.bin_loc = (opts & OPT__BIN_LOC__MASK) >> OPT__BIN_LOC__SHIFT;
    cfg.rtps_mode = (opts & OPT__RTPS_MODE__MASK) >> OPT__RTPS_MODE__SHIFT;
    cfg.subsystems = (opts & OPT__SUBSYS__MASK) >> OPT__SUBSYS__SHIFT;

    print_boot_cfg(&cfg);
    return 0;
}


void boot_request(subsys_t subsys)
{
    printf("BOOT: accepted reboot request for subsystems %s\r\n",
           subsys_name(subsys));
    reboot_requests |= subsys; // coallesce requests
    // TODO: SEV (to prevent race between requests check and WFE in main loop)
}

bool boot_pending()
{
    return !!reboot_requests;
}

int boot_handle(subsys_t *subsys)
{
    int b = 0;
    while (b < NUM_SUBSYSS && !(reboot_requests & (1 << b)))
        b++;
    if (b == NUM_SUBSYSS)
        return 1;
    *subsys = (subsys_t)(1 << b);
    return 0;
}

int boot_reboot(subsys_t subsys)
{
    int rc = 0;
    printf("BOOT: rebooting subsys %s...\r\n", subsys_name(subsys));

    rc |= boot_load(subsys);
    rc |= boot_reset(subsys);

    reboot_requests &= ~subsys;
    printf("BOOT: rebooted subsys %s: rc %u\r\n", subsys_name(subsys), rc);
   return rc;
}
