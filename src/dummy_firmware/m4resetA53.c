#include <stdint.h>
#include "uart.h"

#define APU_RESET_ADDR 0xfd1a0104
#define APU_RESET_VALUE 0x800000fe

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    puts("TRCH\n");

    // Turn on/reset the A53 cluster.
    *((uint32_t *)APU_RESET_ADDR) = APU_RESET_VALUE;

    while (1) {
        asm("wfi");
    }
}

int irq0 (void) {
    puts("IRQ 0\n");
    return(0);
}

int irq1 (void) {
    puts("IRQ 1\n");
    return(0);
}

int irq2 (void) {
    puts("IRQ 2\n");
    return(0);
}

int irq3 (void) {
    puts("IRQ 3\n");
    return(0);
}

int irq4 (void) {
    puts("IRQ 4\n");
    return(0);
}

int irq5 (void) {
    puts("IRQ 5\n");
    return(0);
}

int irq6 (void) {
    puts("IRQ 6\n");
    return(0);
}

int irq7 (void) {
    puts("IRQ 7\n");
    return(0);
}

int irq8 (void) {
    puts("IRQ 8\n");
    return(0);
}

int irq9 (void) {
    puts("IRQ 9\n");
    return(0);
}

int irq10 (void) {
    puts("IRQ 10\n");
    return(0);
}

int irq11 (void) {
    puts("IRQ 11\n");
    return(0);
}

int irq12 (void) {
    puts("IRQ 12\n");
    return(0);
}

int irq13 (void) {
    puts("IRQ 13\n");
    return(0);
}

int irq14 (void) {
    puts("IRQ 14\n");
    return(0);
}

int irq15 (void) {
    puts("IRQ 15\n");
    return(0);
}

int irq16 (void) {
    puts("IRQ 16\n");
    return(0);
}

int irq17 (void) {
    puts("IRQ 17\n");
    return(0);
}

int irq18 (void) {
    puts("IRQ 18\n");
    return(0);
}

int irq19 (void) {
    puts("IRQ 19\n");
    return(0);
}

