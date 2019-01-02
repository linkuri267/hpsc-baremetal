#include "printf.h"
#include "reset.h"
#include "mem-map.h"
#include "smc.h"

#include "boot.h"

static subsys_t reboot_requests;

struct config {
    enum {
        CFG__BOOT_MODE__SRAM = 0x1, // load HPPS/RTPS binaries from SRAM
        CFG__BOOT_MODE__DRAM = 0x2, // assume HPPS/RTPS binaries already in DRAM
    } boot_mode;
    enum {
        CFG__RTPS_MODE__SPLIT = 0x0,
        CFG__RTPS_MODE__LOCKSTEP = 0x1,
        CFG__RTPS_MODE__SMP = 0x2,
    } rtps_mode;
};

static struct config cfg;

static int boot_load(subsys_t subsys)
{
    if (cfg.boot_mode == CFG__BOOT_MODE__DRAM) {
        printf("BOOT: nothing to load, according to boot cfg\r\n");
        return 0;
    }

    if (cfg.boot_mode != CFG__BOOT_MODE__SRAM) {
        printf("BOOT: don't know how to load: invalid boot cfg\r\n");
        return 1;
    }

    switch (subsys) {
        case SUBSYS_RTPS:
            printf("BOOT: load RTPS\r\n");
            if (smc_sram_load("rtps-bl"))
                return 1;
            if (smc_sram_load("rtps-os"))
                return 1;
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

int boot_config()
{
    cfg.boot_mode = *(uint32_t *)TRCH_BOOT_CFG__BOOT_MODE;
    cfg.rtps_mode = *(uint32_t *)TRCH_BOOT_CFG__RTPS_MODE;

    printf("BOOT: cfg: boot mode %u rtps mode %u\r\n",
           cfg.boot_mode, cfg.rtps_mode);
    return 0;
}


void boot_request(subsys_t subsys)
{
    printf("BOOT: accepted reboot request for subsystem %u\r\n", subsys);
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
    printf("BOOT: rebooting subsys %u...\r\n", subsys);

    rc |= boot_load(subsys);
    rc |= reset_release(subsys);

    reboot_requests &= ~subsys;
    printf("BOOT: rebooted subsys %u: rc %u\r\n", subsys, rc);
   return rc;
}
