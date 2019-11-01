#include <stdint.h>

#include "hwinfo.h"
#include "console.h"

#include "ns16550.h"

int console_init()
{
    return ns16550_startup(CONFIG_UART_BASE, UART_CLOCK, CONFIG_UART_BAUDRATE);
}

void _putchar(char c)
{
    ns16550_putchar(c);
}
