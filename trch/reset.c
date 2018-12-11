#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "regops.h"
#include "subsys.h"

#if TEST_BOOT_FROM_SMC_SRAM
#include "smc.h"
#endif // TEST_BOOT_FROM_SMC_SRAM

#include "reset.h"

#define APU       ((volatile uint8_t *)0xfd5c0000)
#define CRF       ((volatile uint8_t *)0xfd1a0000)
#define CRL       ((volatile uint8_t *)0xff5e0000)
#define RPU_CTRL  ((volatile uint8_t *)0xff9a0000)

#define APU__PWRCTL 0x90
#define APU__PWRCTL__CPUPWRDWNREQ 0xff

#define CRF__RST_FPD_APU 0x104
#define CRF__RST_FPD_APU__ACPU0_RESET 0xff

#define CRL__RST_LPD_TOP 0x23c
#define CRL__RST_LPD_TOP__RPU_R50_RESET 0x3

#define RPU_CTRL__RPU_0_CFG  0x100
#define RPU_CTRL__RPU_0_CFG__NCPUHALT 0x1

#define RPU_CTRL__RPU_1_CFG  0x200
#define RPU_CTRL__RPU_1_CFG__NCPUHALT 0x1

int reset_subsys(subsys_t subsys)
{
    switch (subsys) {
        case SUBSYS_HPPS:
            printf("RESET: HPPS: halt CPU0-7, release CPU0\r\n");

            REGB_SET32(APU, APU__PWRCTL, APU__PWRCTL__CPUPWRDWNREQ);
            REGB_SET32(CRF, CRF__RST_FPD_APU, CRF__RST_FPD_APU__ACPU0_RESET);

#if TEST_BOOT_FROM_SMC_SRAM
            if (smc_sram_load("hpps-fw"))
                return 1;
            if (smc_sram_load("hpps-bl"))
                return 1;
#endif // TEST_BOOT_FROM_SMC_SRAM

            REGB_CLEAR32(APU, APU__PWRCTL, APU__PWRCTL__CPUPWRDWNREQ & 0x1);
            REGB_CLEAR32(CRF, CRF__RST_FPD_APU, CRF__RST_FPD_APU__ACPU0_RESET & 0x1);
            break;
        case SUBSYS_RTPS:
            printf("RESET: RTPS: halt CPU0-1, release CPU0\r\n");

            REGB_SET32(CRL, CRL__RST_LPD_TOP, CRL__RST_LPD_TOP__RPU_R50_RESET);
            REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_0_CFG, RPU_CTRL__RPU_0_CFG__NCPUHALT);
            REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_1_CFG, RPU_CTRL__RPU_1_CFG__NCPUHALT);

#if TEST_BOOT_FROM_SMC_SRAM
            if (smc_sram_load("rtps-bl"))
                return 1;
            if (smc_sram_load("rtps-os"))
                return 1;
#endif // TEST_BOOT_FROM_SMC_SRAM

            REGB_SET32(RPU_CTRL, RPU_CTRL__RPU_0_CFG, RPU_CTRL__RPU_0_CFG__NCPUHALT);
            REGB_CLEAR32(CRL, CRL__RST_LPD_TOP, CRL__RST_LPD_TOP__RPU_R50_RESET & 0x1);
            break;
        default:
            return 1;
     };
    return 0;
}
