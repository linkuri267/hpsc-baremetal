#include <stdint.h>

#include "hwinfo.h"
#include  "console.h"

#if defined(CONFIG_CONSOLE__NS16550)

#include "ns16550.h"

int console_init()
{
    return ns16550_startup(CONFIG_UART_BASE, UART_CLOCK, CONFIG_UART_BAUDRATE);
}

void _putchar(char c)
{
    ns16550_putchar(c);
}

#elif defined(CONFIG_CONSOLE__CADENCE)

#include "cadence_uart.h"

int console_init()
{
    return cdns_uart_startup(CONFIG_UART_BASE);
}

void _putchar(char c)
{
    cdns_uart_poll_put_char(c);
}

#else // CONFIG_CONSOLE__*
#error Invalid console choice: see CONFIG_CONSOLE
#endif // CONFIG_CONSOLE__*
