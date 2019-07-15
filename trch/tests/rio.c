#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "hwinfo.h"
#include "nvic.h"
#include "test.h"

volatile uint32_t payload = 0;

int test_rio()
{
    nvic_int_enable(TRCH_IRQ__RIO_1);

    *((volatile uint32_t *)0xe3000000) = 0xdeadbe01;
    printf("test rio: waiting for payload\r\n");
#if 1
    while (payload == 0);
    printf("test rio: payload received: %u\r\n", payload);
#endif
    return 0;
}

void rio_1_isr()
{
    nvic_int_disable(TRCH_IRQ__RIO_1);
    payload = *((volatile uint32_t *)0xe4000004);
}
