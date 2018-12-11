#include "printf.h"
#include "reset.h"
#include "smc.h"

#include "boot.h"

static subsys_t reboot_requests;

static int boot_load(subsys_t subsys)
{
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
            break;
        default:
            printf("BOOT: ERROR: unknown subsystem %x\r\n", subsys);
            return 1;
     };
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
    while (reboot_requests) {
        printf("BOOT: performing requested reboots...\r\n");

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
   return rc;
}
