#include <stdint.h>

#include <libmspprintf/printf.h>

#include "uart.h"
#include "nvic.h"
#include "mailbox.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_VALUE 0x800000fe

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\n");

    /* TODO: This doesn't make a difference: access succeeds without this */
    printf("PRV: svc #0\n");
    asm("svc #0");

    nvic_int_enable(MBOX_HAVE_DATA_IRQ);

    // Turn on/reset the A53 cluster.
    printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_VALUE);
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;

    while (1) {
        asm("wfi");
    }
}
