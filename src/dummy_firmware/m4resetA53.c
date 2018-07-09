#include <stdint.h>
#include "uart.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_VALUE 0x800000fe

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    puts("TRCH\n");

    // Turn on/reset the A53 cluster.
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;

    while (1) {
        asm("wfi");
    }
}
