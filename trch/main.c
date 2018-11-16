#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
#include "reset.h"
#include "command.h"
#include "mmu.h"
#include "panic.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "mailbox.h"
#include "hwinfo.h"
#include "intc.h"
#include "test.h"

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\r\n");

    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

    intc_init((volatile uint32_t *)TRCH_NVIC_BASE);

#if TEST_FLOAT
    if (test_float())
        panic("float test");
#endif // TEST_FLOAT

#if TEST_TRCH_DMA
    if (test_trch_dma())
        panic("TRCH DMA test");
#endif // TEST_TRCH_DMA

#if TEST_RT_MMU
    if (test_rt_mmu())
        panic("RTPS/TRCH-HPPS MMU test");
#endif // TEST_RT_MMU

#if TEST_HPPS_TRCH_MAILBOX_SSW
    struct mbox_link *hpps_link_ssw = mbox_link_connect(
                    MBOX_HPPS_TRCH__BASE, MBOX_HPPS_TRCH__IRQ_START,
                    MBOX_HPPS_TRCH__HPPS_TRCH_SSW, MBOX_HPPS_TRCH__TRCH_HPPS_SSW,
                    MBOX_HPPS_TRCH__TRCH_RCV_INT, MBOX_HPPS_TRCH__TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link_ssw)
        panic("HPPS link SSW");

    // Never release the link, because we listen on it in main loop
#endif

#if TEST_HPPS_TRCH_MAILBOX
    struct mbox_link *hpps_link = mbox_link_connect(
                    MBOX_HPPS_TRCH__BASE, MBOX_HPPS_TRCH__IRQ_START,
                    MBOX_HPPS_TRCH__HPPS_TRCH, MBOX_HPPS_TRCH__TRCH_HPPS,
                    MBOX_HPPS_TRCH__TRCH_RCV_INT, MBOX_HPPS_TRCH__TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        panic("HPPS link");

    // Never release the link, because we listen on it in main loop
#endif

#if TEST_RTPS_TRCH_MAILBOX
    struct mbox_link *rtps_link = mbox_link_connect(
                    MBOX_LSIO__BASE, MBOX_LSIO__IRQ_START,
                    MBOX_LSIO__RTPS_TRCH, MBOX_LSIO__TRCH_RTPS,
                    MBOX_LSIO__TRCH_RCV_INT, MBOX_LSIO__TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        panic("RTPS link");

    // Never disconnect the link, because we listen on it in main loop
#endif // TEST_RTPS_TRCH_MAILBOX

#if TEST_RTPS
    reset_component(COMPONENT_RTPS);
#endif // TEST_RTPS

#if TEST_HPPS
    reset_component(COMPONENT_HPPS);
#endif // TEST_HPPS

#if TEST_IPI
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
        while (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // endif

        // If interrupt happens after the cmd queueu check above, then ISR will
        // run and event register will be automatically set, so won't sleep here.
        printf("Waiting for event...\r\n");
        asm("wfe");
    }
}
