#define DEBUG 0

#include <stdint.h>
#include <stdbool.h>

#include "hwinfo.h"
#include "printf.h"
#include "regops.h"
#include "subsys.h"
#include "sleep.h"

#include "reset.h"

#define APU__PWRCTL 0x90
#define APU__PWRCTL__CPUxPWRDWNREQ__SHIFT               0
#define APU__PWRCTL__CPUxPWRDWNREQ                   0xff

#define CRF__RST_FPD_APU 0x104
#define CRF__RST_FPD_APU__ACPUx_RESET__SHIFT            0
#define CRF__RST_FPD_APU__ACPUx_RESET                0xff
#define CRF__RST_FPD_APU__GIC_RESET               0x40000

#define CRL__RST_LPD_TOP 0x23c
#define CRL__RST_LPD_TOP__RPU_R5x_RESET__SHIFT          0
#define CRL__RST_LPD_TOP__RPU_R5x_RESET               0x3
#define CRL__RST_LPD_TOP__RPU_A53_RESET               0x4
#define CRL__RST_LPD_TOP__GIC_RESET            0x01000000
#define CRL__RST_LPD_TOP__A53_GIC_RESET        0x02000000

#define RPU_CTRL__RPU_GLBL_CNTL	0x0
#define RPU_CTRL__RPU_GLBL_CNTL__SLSPLIT 0x8

#define RPU_CTRL__RPU_0_CFG  0x100
#define RPU_CTRL__RPU_0_CFG__NCPUHALT 0x1

#define RPU_CTRL__RPU_1_CFG  0x200
#define RPU_CTRL__RPU_1_CFG__NCPUHALT 0x1

#define RPU_CTRL__RPU_2_CFG  0x270
#define RPU_CTRL__RPU_2_CFG__NCPUHALT 0x1

int reset_assert(comp_t comps)
{
    printf("RESET: assert: components mask %x\r\n", comps);

    // Note: we tie the GICs to the respective CPUs unconditionally here

    if (comps & COMP_CPUS_RTPS) {
        REGB_SET32(CRL, CRL__RST_LPD_TOP,
                ((CRL__RST_LPD_TOP__RPU_R5x_RESET | CRL__RST_LPD_TOP__RPU_A53_RESET) &
                        (((comps & COMP_CPUS_RTPS) >> COMP_CPUS_SHIFT_RTPS)
                                << CRL__RST_LPD_TOP__RPU_R5x_RESET__SHIFT)) |
                CRL__RST_LPD_TOP__GIC_RESET | CRL__RST_LPD_TOP__A53_GIC_RESET);
    }
    if (comps & COMP_CPU_RTPS_0)
        REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_0_CFG, RPU_CTRL__RPU_0_CFG__NCPUHALT);
    if (comps & COMP_CPU_RTPS_1)
        REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_1_CFG, RPU_CTRL__RPU_1_CFG__NCPUHALT);
    if (comps & COMP_CPU_RTPS_A53_0)
        REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_2_CFG, RPU_CTRL__RPU_2_CFG__NCPUHALT);

    // on assert, always clear the mode to lockstep, see also comment in release
    if (comps & COMP_CPUS_RTPS) {
        REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_GLBL_CNTL, RPU_CTRL__RPU_GLBL_CNTL__SLSPLIT);
        REGB_CLEAR32(RPU_CTRL, RPU_CTRL__RPU_GLBL_CNTL, RPU_CTRL__RPU_GLBL_CNTL__SLSPLIT);
    }
    if (comps & COMP_CPUS_HPPS) {
        REGB_SET32(CRF, CRF__RST_FPD_APU,
                   (CRF__RST_FPD_APU__ACPUx_RESET &
                    (((comps & COMP_CPUS_HPPS) >> COMP_CPUS_SHIFT_HPPS)
                        << CRF__RST_FPD_APU__ACPUx_RESET__SHIFT)) |
                   CRF__RST_FPD_APU__GIC_RESET);
        REGB_SET32(APU, APU__PWRCTL,
               (APU__PWRCTL__CPUxPWRDWNREQ &
                (((comps & COMP_CPUS_HPPS) >> COMP_CPUS_SHIFT_HPPS)
                    << APU__PWRCTL__CPUxPWRDWNREQ__SHIFT)));
    }
    return 0;
}

int reset_release(comp_t comps)
{
    printf("RESET: release: components mask %x\r\n", comps);

    // Note: we tie the GICs to the respective CPUs unconditionally here

    // The only time we request both CPUs is in SPLIT mode. If this stops being
    // true, then add a method reset_set_rtps_mode and call it from boot.c
    if ((comps & COMP_CPUS_RTPS_R52) == COMP_CPUS_RTPS_R52)
        REGB_SET32(RPU_CTRL, RPU_CTRL__RPU_GLBL_CNTL, RPU_CTRL__RPU_GLBL_CNTL__SLSPLIT);

    if (comps & COMP_CPU_RTPS_0)
        REGB_SET32(RPU_CTRL, RPU_CTRL__RPU_0_CFG, RPU_CTRL__RPU_0_CFG__NCPUHALT);
    if (comps & COMP_CPU_RTPS_1)
        REGB_SET32(RPU_CTRL, RPU_CTRL__RPU_1_CFG, RPU_CTRL__RPU_1_CFG__NCPUHALT);
    if (comps & COMP_CPU_RTPS_A53_0)
        REGB_SET32(RPU_CTRL, RPU_CTRL__RPU_2_CFG, RPU_CTRL__RPU_2_CFG__NCPUHALT); 

    uint32_t reg_val;
    switch (comps & COMP_CPUS_RTPS) {
        case COMP_CPUS_RTPS:
            reg_val = ((CRL__RST_LPD_TOP__RPU_R5x_RESET | CRL__RST_LPD_TOP__RPU_A53_RESET) &
                      (((COMP_CPUS_RTPS) >> COMP_CPUS_SHIFT_RTPS)
                      << CRL__RST_LPD_TOP__RPU_R5x_RESET__SHIFT));
            break;
        case COMP_CPU_RTPS_0:
        case COMP_CPU_RTPS_1:
        case COMP_CPUS_RTPS_R52:
            reg_val = ((CRL__RST_LPD_TOP__RPU_R5x_RESET) &
                      (((comps & COMP_CPUS_RTPS_R52) >> COMP_CPUS_SHIFT_RTPS)
                      << CRL__RST_LPD_TOP__RPU_R5x_RESET__SHIFT));
            break;
        case COMP_CPUS_RTPS_A53:
            reg_val = (((COMP_CPUS_RTPS_A53) >> COMP_CPUS_SHIFT_RTPS)
                      << CRL__RST_LPD_TOP__RPU_R5x_RESET__SHIFT);
            break;
        default:
            reg_val = 0;
    }
    if ((comps & COMP_CPUS_RTPS)) {
        REGB_CLEAR32(CRL, CRL__RST_LPD_TOP, reg_val);

        mdelay(1); // wait for GIC at least 5 cycles (GIC-500 TRM Table A-1)
        if ((comps & COMP_CPUS_RTPS_R52))
            REGB_CLEAR32(CRL, CRL__RST_LPD_TOP, CRL__RST_LPD_TOP__GIC_RESET);
        if ((comps & COMP_CPUS_RTPS_A53))
            REGB_CLEAR32(CRL, CRL__RST_LPD_TOP, CRL__RST_LPD_TOP__A53_GIC_RESET);
    }

    if (comps & COMP_CPUS_HPPS) {
        REGB_CLEAR32(APU, APU__PWRCTL,
                (APU__PWRCTL__CPUxPWRDWNREQ &
                 (((comps & COMP_CPUS_HPPS) >> COMP_CPUS_SHIFT_HPPS)
                  << APU__PWRCTL__CPUxPWRDWNREQ__SHIFT)));

        REGB_CLEAR32(CRF, CRF__RST_FPD_APU, CRF__RST_FPD_APU__GIC_RESET);
        mdelay(1); // wait for GIC at least 5 cycles (GIC-500 TRM Table A-1)

        REGB_CLEAR32(CRF, CRF__RST_FPD_APU,
                (CRF__RST_FPD_APU__ACPUx_RESET &
                 (((comps & COMP_CPUS_HPPS) >> COMP_CPUS_SHIFT_HPPS)
                  << CRF__RST_FPD_APU__ACPUx_RESET__SHIFT)));
    }
    return 0;
}
