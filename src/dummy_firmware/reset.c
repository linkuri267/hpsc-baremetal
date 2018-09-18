#include <stdint.h>
#include <stdbool.h>

#include "printf.h"

#include "reset.h"

#define APU       ((volatile uint8_t *)0xfd5c0000)
#define CRF       ((volatile uint8_t *)0xfd1a0000)
#define CRL       ((volatile uint8_t *)0xff5e0000)
#define RPU_CTRL  ((volatile uint8_t *)0xff9a0000)

#define APU__PWRCTL 0x90
#define APU__PWRCTL__CPUPWRDWNREQ 0x1

#define CRF__RST_FPD_APU 0x104
#define CRF__RST_FPD_APU__ACPU0_RESET 0x1

#define CRL__RST_LPD_TOP 0x23c
#define CRL__RST_LPD_TOP__RPU_R50_RESET 0x1

#define RPU_CTRL__RPU_0_CFG  0x100
#define RPU_CTRL__RPU_0_CFG__NCPUHALT 0x1

static const char *component_name(component_t c)
{
    switch (c) {
        case COMPONENT_RTPS: return "RTPS";
        case COMPONENT_HPPS: return "HPPS";
        default: return "?";
   }
}

void reset_component(component_t component)
{
    volatile uint32_t *reg_addr;
    uint32_t reg_val;

    switch (component) {
        case COMPONENT_HPPS:
            reg_addr = (volatile uint32_t *)(APU + APU__PWRCTL);
            reg_val = APU__PWRCTL__CPUPWRDWNREQ;

            printf("reset: %s: %p <&- ~0x%08lx\r\n", component_name(component), reg_addr, reg_val);
            *reg_addr &= ~reg_val;

            reg_addr = (volatile uint32_t *)(CRF + CRF__RST_FPD_APU);
            reg_val = CRF__RST_FPD_APU__ACPU0_RESET;

            printf("reset: %s: %p <&- ~0x%08lx\r\n", component_name(component), reg_addr, reg_val);
            *reg_addr &= ~reg_val;


            break;
        case COMPONENT_RTPS:
            reg_addr = (volatile uint32_t *)(CRL + CRL__RST_LPD_TOP);
            reg_val = CRL__RST_LPD_TOP__RPU_R50_RESET;

            printf("reset: %s: %p <&- ~0x%08lx\r\n", component_name(component), reg_addr, reg_val);
            *reg_addr &= ~reg_val;

            reg_addr = (volatile uint32_t *)(RPU_CTRL + RPU_CTRL__RPU_0_CFG);
            reg_val = RPU_CTRL__RPU_0_CFG__NCPUHALT;

            printf("clear halt: %s: %p <|- 0x%08lx\r\n", component_name(component), reg_addr, reg_val);
            *reg_addr |= reg_val;

            break;
   };
}
