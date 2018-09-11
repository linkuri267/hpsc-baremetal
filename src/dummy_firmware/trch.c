#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
#include "float.h"
#include "nvic.h"
#include "mailbox.h"
#include "reset.h"
#include "command.h"

#define TEST_RTPS
#define TEST_HPPS

#define TEST_FLOAT
// #define TEST_HPPS_TRCH_MAILBOX
// #define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\r\n");

    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

#ifdef TEST_RTPS
    reset_component(COMPONENT_RTPS, /* first boot */ true);
#endif // TEST_RTPS

#ifdef TEST_HPPS
    reset_component(COMPONENT_HPPS, /* first_boot */ true);
#endif // TEST_HPPS

#ifdef TEST_FLOAT
    printf("Testing float...\r\n");
    float_test();
#endif // TEST_FLOAT

#ifdef TEST_HPPS_TRCH_MAILBOX
    printf("Testing HPPS-TRCH mailbox...\r\n");
    mbox_init(HPPS_TRCH_MBOX_BASE);
    nvic_int_enable(HPPS_TRCH_MBOX_HAVE_DATA_IRQ);
#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    printf("Testing RTPS-TRCH mailbox...\r\n");
    mbox_init(RTPS_TRCH_MBOX_BASE);
    nvic_int_enable(RTPS_TRCH_MBOX_HAVE_DATA_IRQ);

    /* after some delay */
    int total = 0;
    int i, j;
    for (i = 0; i < 1000000; i++) {
      for (j = 0; j < 1000; j++)
        total++;
    }
    printf("M4: is the delay long enough?\n");

    printf("M4: send message to R52\n");
    for (i = 0; i < 15; i++) 
       mbox_send(RTPS_TRCH_MBOX_BASE, i);
#endif // TEST_FLOAT

#ifdef TEST_IPI
    printf("Testing IPI...\r\n");
    // uint32_t * addr = (unsigned int *) 0xff310000; /* IPI_TRIG */
    uint32_t val = (1 << 8); 	/* RPU0 */
    * ((uint32_t *) 0xff300000)= val;
    * ((uint32_t *) 0xff310000)= val;
    * ((uint32_t *) 0xff320000)= val;
    * ((uint32_t *) 0xff380000)= val;
    printf("M4: after trigger interrupt to R52\n");
#endif // TEST_IPI

    while (1) {
        asm("wfi");
    }
}
