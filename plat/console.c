#include <stdint.h>

#include "hwinfo.h"
#include  "console.h"

#if defined(CONFIG_CONSOLE__CADENCE)

#include "cadence_uart.h"

int console_init()
{
    return cdns_uart_startup(UART_BASE);
}

void _putchar(char c)
{
    cdns_uart_poll_put_char(c);
}

#else // CONFIG_CONSOLE__*
#error Invalid console choice: see CONFIG_CONSOLE
#endif // CONFIG_CONSOLE__*
