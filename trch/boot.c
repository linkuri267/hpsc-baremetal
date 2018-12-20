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


void boot_request_reboot(subsys_t subsys)
{
    printf("BOOT: accepted reboot request for subsystem %u\r\n", subsys);
    reboot_requests |= subsys; // coallesce requests
    // TODO: SEV (to prevent race between requests check and WFE in main loop)
}

int boot_perform_reboots()
{
    int rc = 0;
    int reqs = 0;
    while (reboot_requests) {
        printf("BOOT: performing requested reboots...\r\n");
        reqs++;

        int b = 0;
        while (b < NUM_SUBSYSS && !(reboot_requests & (1 << b)))
            b++;

        if (reboot_requests & (1 << b)) {
            subsys_t comp = (subsys_t)(1 << b);

            rc |= boot_load(comp);
            rc |= reset_release(comp);

            reboot_requests &= ~comp;
        }
   }
   if (reqs)
       printf("BOOT: completed %u boot requests: rc %u\r\n", reqs, rc);
   return rc;
}
