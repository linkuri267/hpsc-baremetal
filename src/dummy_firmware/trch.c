#include <stdint.h>
#include <stdbool.h>

#include <libmspprintf/printf.h>

#include "uart.h"
#include "nvic.h"
#include "mailbox.h"
#include "reset.h"
#include "command.h"

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\n");

    /* TODO: This doesn't make a difference: access succeeds without this */
    printf("PRV: svc #0\n");
    asm("svc #0");

    nvic_int_enable(MBOX_HAVE_DATA_IRQ);
    reset_hpps(/* first_boot */ true);

    while (1) {
        asm("wfi");
    }
}
