#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
#include "float.h"
#include "reset.h"
#include "command.h"
#include "mmu.h"
#include "panic.h"
#include "test-mailbox.h"

#define TEST_RTPS
#define TEST_HPPS

// #define TEST_FLOAT
#define TEST_HPPS_TRCH_MAILBOX
#define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI
// #define TEST_RTPS_HPPS_MMU

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\r\n");

    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

#ifdef TEST_FLOAT
    printf("Testing float...\r\n");
    float_test();
#endif // TEST_FLOAT

#ifdef TEST_HPPS_TRCH_MAILBOX
    setup_hpps_trch_mailbox();
#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    setup_rtps_trch_mailbox();
#endif // TEST_RTPS_TRCH_MAILBOX

#ifdef TEST_RTPS_HPPS_MMU
    mmu_init((void *)0xc0000000, 0x100000000);
#endif // TEST_RTPS_HPPS_MMU

#ifdef TEST_RTPS
    reset_component(COMPONENT_RTPS);
#endif // TEST_RTPS

#ifdef TEST_HPPS
    reset_component(COMPONENT_HPPS);
#endif // TEST_HPPS

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

        //printf("main\r\n");

#if defined(TEST_RTPS_TRCH_MAILBOX) || defined(TEST_HPPS_TRCH_MAILBOX)
        struct cmd cmd;
        if (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // endif

        printf("Waiting for interrupt...\r\n");
        asm("wfi");
    }
}
