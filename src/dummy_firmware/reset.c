#include <stdint.h>
#include <stdbool.h>

#include "printf.h"

#include "reset.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_PWRON_VALUE 0x3fdff
#define APU_RESET_VALUE 0x800000fe

#define RPU_RESET_ADDR 0xff5e023c
#define RPU_RESET_VALUE 0x188fde

static const char *component_name(component_t c)
{
    switch (c) {
        case COMPONENT_RTPS: return "RTPS";
        case COMPONENT_HPPS: return "HPPS";
        default: return "?";
   }
}

void reset_component(component_t component, bool first_boot)
{
    volatile uint32_t *reg_addr;
    uint32_t reg_val, pwr_on_reg_val;

    switch (component) {
        case COMPONENT_HPPS:
            reg_addr = (volatile uint32_t *)APU_RESET_ADDR;
            reg_val = APU_RESET_VALUE;
            pwr_on_reg_val = APU_RESET_PWRON_VALUE;
            break;
        case COMPONENT_RTPS:
            reg_addr = (volatile uint32_t *)RPU_RESET_ADDR;
            reg_val = RPU_RESET_VALUE;
            break;
   };
    // For subsequent reset to happen, need to toggle to pwron reg_val before clearing the bits
    // TODO: A reset after boot does happen, but kernel fails to boot a second time
    if (!first_boot) {
        printf("reset: %s: %p <- 0x%08lx\r\n", component_name(component), reg_addr, pwr_on_reg_val);
        *reg_addr = pwr_on_reg_val;
    }

    printf("reset: %s: %p <- 0x%08lx\r\n", component_name(component), reg_addr, reg_val);
    *reg_addr = reg_val;
}
