#define putc cdns_uart_poll_put_char
#define puts cdns_uart_poll_puts

int cdns_uart_startup(uintptr_t base);
void cdns_uart_poll_put_char(unsigned char c);
void cdns_uart_poll_puts(const char *c);

