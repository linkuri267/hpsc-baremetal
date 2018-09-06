#include <stdint.h>
#include <stdbool.h>

#include <libmspprintf/printf.h>

#include "reset.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_PWRON_VALUE 0x3fdff
#define APU_RESET_VALUE 0x800000fe

void reset_hpps(bool first_boot)
{
    // For subsequent reset to happen, need to toggle to pwron value before clearing the bits
    // TODO: A reset after boot does happen, but kernel fails to boot a second time
    if (!first_boot) {
        printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_PWRON_VALUE);
        *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_PWRON_VALUE;
    }

    printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_VALUE);
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;
}

#define RPU_RESET_ADDR 0xff5e023c
#define RPU_RESET_VALUE 0x188fde

void reset_r52(bool first_boot)
{
    // For subsequent reset to happen, need to toggle to pwron value before clearing the bits
    // TODO: A reset after boot does happen, but kernel fails to boot a second time
/*
    if (!first_boot) {
        printf("R52 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)RPU_RESET_ADDR), RPU_RESET_PWRON_VALUE);
        *((volatile uint32_t *)RPU_RESET_ADDR) = RPU_RESET_PWRON_VALUE;
    }
*/
    printf("R52 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)RPU_RESET_ADDR), RPU_RESET_VALUE);
    *((volatile uint32_t *)RPU_RESET_ADDR) = RPU_RESET_VALUE;
}

