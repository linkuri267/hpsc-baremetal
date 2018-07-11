#include <stdint.h>

#include <libmspprintf/printf.h>

#include "command.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_PWRON_VALUE 0x3fdff
#define APU_RESET_VALUE 0x800000fe

void cmd_reset_hpps()
{
    // For subsequent reset to happen, need to toggle to pwron value before clearing the bits
    // TODO: A reset after boot does happen, but kernel fails to boot a second time
    printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_VALUE);
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_PWRON_VALUE;

    printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_PWRON_VALUE);
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;
}

void cmd_handle(unsigned cmd)
{
    printf("CMD %x\n", cmd);
    switch (cmd) {
        case CMD_RESET_HPPS:
            cmd_reset_hpps();
            break;
        default:
            printf("ERROR: unknown cmd: %x\n", cmd);
    }
}
