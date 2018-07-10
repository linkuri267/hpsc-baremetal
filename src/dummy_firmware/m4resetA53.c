#include <stdint.h>

#include <libmspprintf/printf.h>

#include "uart.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_VALUE 0x800000fe

#define NVIC_BASE 0xe000e000
#define NVIC_ISER0_OFFSET 0x100

#define MBOX_IRQ 162 /* External IRQ numbering (i.e. vector #16 has index 0). */

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\n");

    /* TODO: This doesn't make a difference: access succeeds without this */
    printf("PRV: svc #0\n");
    asm("svc #0");

    // Enable interrupt from mailbox
    volatile uint32_t *nvic_reg_ie = (volatile uint32_t *)(NVIC_BASE + NVIC_ISER0_OFFSET + (MBOX_IRQ/32)*4);
    uint32_t nvic_reg_ie_val = 1 << (MBOX_IRQ % 32);

    printf("NVIC IE: %p <- 0x%08lx\n", nvic_reg_ie, nvic_reg_ie_val);
    *nvic_reg_ie |= nvic_reg_ie_val;


    // Turn on/reset the A53 cluster.
    printf("A53 RST: %p <- 0x%08lx\n", ((volatile uint32_t *)APU_RESET_ADDR), APU_RESET_VALUE);
    *((volatile uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;


    while (1) {
        asm("wfi");
    }
}
