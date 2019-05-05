#include "printf.h"
#include "reset.h"
#include "smc.h"
#include "watchdog.h"
#include "syscfg.h"
#include "mem-map.h"

#include "boot.h"

static subsys_t reboot_requests;

// For legacy way to configure HPPS u-boot, until have u-boot env/script.
#define HPPS_BOOT_MODE__DRAM 0x0
#define HPPS_BOOT_MODE__NAND 0xf
static int hpps_boot_mode(enum memdev rootfs_loc, volatile uint32_t *mode)
{
    switch (rootfs_loc) {
        case MEMDEV_HPPS_DRAM:     *mode = HPPS_BOOT_MODE__DRAM; break;
        case MEMDEV_HPPS_SMC_NAND: *mode = HPPS_BOOT_MODE__NAND; break;
        default:
                printf("ERROR: BOOT: unsupported HPPS rootfs loc: %s\r\n",
                       memdev_name(rootfs_loc));
                return 1;
    };
    printf("BOOT: set hpps boot mode: %p <- 0x%x\r\n", mode, *mode);
    return 0;
}

static int boot_load(subsys_t subsys, struct syscfg *cfg)
{
    switch (subsys) {
        case SUBSYS_RTPS_R52:
            printf("BOOT: load RTPS mode: %x\r\n", cfg->rtps_mode);

            if (cfg->bin_loc != MEMDEV_TRCH_SMC_SRAM) {
                printf("BOOT: syscfg'ed bins not in SMC SRAM, not loading\r\n");
                return 0;
            }

            switch (cfg->rtps_mode) {
                case SYSCFG__RTPS_MODE__SPLIT: // TODO
                    printf("TODO: NOT IMPLEMENTED: loading for SPLIT mode");
                    break;
                case SYSCFG__RTPS_MODE__LOCKSTEP:
                    if (smc_sram_load("rtps-bl", NULL))
                        return 1;
                    if (smc_sram_load("rtps-os", NULL))
                        return 1;
                    break;
                case SYSCFG__RTPS_MODE__SMP: // TODO
                    printf("TODO: NOT IMPLEMENTED: loading for SMP mode");
                    break;
            }
            break;
        case SUBSYS_RTPS_A53:
            // TODO: load binaries
            break;
        case SUBSYS_HPPS:
            printf("BOOT: load HPPS\r\n");

            // For legacy config of HPPS u-boot, until have u-boot env/script.
            int rc = hpps_boot_mode(cfg->hpps_rootfs_loc,
                                    (volatile uint32_t *)HPPS_BOOT_MODE_ADDR);
            if (rc)
                return 1;

            if (cfg->bin_loc != MEMDEV_TRCH_SMC_SRAM) {
                printf("BOOT: syscfg'ed bins not in SMC SRAM, not loading\r\n");
                return 0;
            }

            if (smc_sram_load("hpps-fw", NULL))
                return 1;
            if (smc_sram_load("hpps-bl", NULL))
                return 1;
            if (smc_sram_load("hpps-bl-dt", NULL)) {
                printf("BOOT: hpps-bl-dt not found in NV mem;"
                       "will fall back to compiled-in DT");
            }
            if (smc_sram_load("hpps-bl-env", NULL)) {
                printf("BOOT: hpps-bl-env not found in NV mem;"
                       "will fall back to compiled-in environment");
            }
            if (smc_sram_load("hpps-dt", NULL))
                return 1;
            if (smc_sram_load("hpps-os", NULL))
                return 1;
            if (smc_sram_load("hpps-initramfs", NULL)) {
                printf("BOOT: hpps-initramfs not found in NV mem;"
                       "booting without initramfs");
            }
            break;
        default:
            printf("BOOT: ERROR: unknown subsystem %x\r\n", subsys);
            return 1;
     };
    return 0;
}

static int boot_reset(subsys_t subsys, struct syscfg *cfg)
{
    int rc = 0;
    switch (subsys) {
        case SUBSYS_RTPS_R52:
            switch (cfg->rtps_mode) {
                case SYSCFG__RTPS_MODE__SPLIT:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52_0);
                    watchdog_init_group(CPU_GROUP_RTPS_R52_1);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__SPLIT);
                    rc |= reset_release(COMP_CPUS_RTPS_R52);
                    break;
                case SYSCFG__RTPS_MODE__LOCKSTEP:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52_0);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__LOCKSTEP);
                    rc = reset_release(COMP_CPU_RTPS_R52_0);
                    break;
                case SYSCFG__RTPS_MODE__SMP:
#if CONFIG_RTPS_R52_WDT
                    watchdog_init_group(CPU_GROUP_RTPS_R52);
#endif // CONFIG_RTPS_R52_WDT
                    reset_set_rtps_r52_mode(RTPS_R52_MODE__SPLIT);
                    rc = reset_release(COMP_CPU_RTPS_R52_0);
                    break;
                default:
                    printf("BOOT: ERROR: unknown RTPS boot mode: %x\r\n",
                           cfg->rtps_mode);
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

int boot_reboot(subsys_t subsys, struct syscfg *cfg)
{
    int rc = 0;
    printf("BOOT: rebooting subsys %s...\r\n", subsys_name(subsys));

    rc |= boot_load(subsys, cfg);
    rc |= boot_reset(subsys, cfg);

    reboot_requests &= ~subsys;
    printf("BOOT: rebooted subsys %s: rc %u\r\n", subsys_name(subsys), rc);
   return rc;
}
