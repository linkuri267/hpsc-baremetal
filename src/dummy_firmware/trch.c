#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
#include "float.h"
#include "nvic.h"
#include "mailbox.h"
#include "reset.h"
#include "command.h"
#include "busid.h"
#include "mmu.h"

#define TEST_RTPS
#define TEST_HPPS

#define TEST_FLOAT
#define TEST_HPPS_TRCH_MAILBOX
#define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI
#define TEST_RTPS_HPPS_MMU

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
    printf("Testing HPPS-TRCH mailbox...\r\n");
    mbox_init_server(HPPS_TRCH_MBOX_BASE, /* instance */ 0, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0, cmd_handle, NULL);
    nvic_int_enable(HPPS_TRCH_MAILBOX_IRQ_A);
#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    printf("Testing RTPS-TRCH mailbox...\r\n");
    mbox_init_server(RTPS_TRCH_MBOX_BASE, /* instance */ 0, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0, cmd_handle, NULL);
    nvic_int_enable(RTPS_TRCH_MAILBOX_IRQ_A);
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
        asm("wfi");
    }
}

void mbox_rtps_isr()
{
    mbox_isr(RTPS_TRCH_MBOX_BASE, 0);
}

void mbox_hpps_isr()
{
    mbox_isr(HPPS_TRCH_MBOX_BASE, 0);
}
