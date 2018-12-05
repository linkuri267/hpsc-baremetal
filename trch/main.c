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
#include "nvic.h"
#include "server.h"
#include "watchdog.h"
#include "test.h"
#include "sram.h"

#define SERVER (TEST_HPPS_TRCH_MAILBOX_SSW || TEST_HPPS_TRCH_MAILBOX || TEST_RTPS_TRCH_MAILBOX)

#if SERVER
typedef enum {
    ENDPOINT_HPPS = 0,
    ENDPOINT_LSIO,
} endpoint_t;

struct endpoint endpoints[] = {
    [ENDPOINT_HPPS] = { .base = MBOX_HPPS_TRCH__BASE },
    [ENDPOINT_LSIO] = { .base = MBOX_LSIO__BASE },
};
#endif // SERVER

int notmain ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\r\n");

    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

    nvic_init((volatile uint32_t *)TRCH_NVIC_BASE);

#if TEST_TRCH_WDT_STANDALONE
    if (test_trch_wdt())
        panic("TRCH WDT test");
#endif // TEST_TRCH_WDT_STANDALONE

#if TEST_RTPS_WDT_STANDALONE
    if (test_rtps_wdt())
        panic("RTPS WDT test");
#endif // TEST_RTPS_WDT_STANDALONE

#if TEST_HPPS_WDT_STANDALONE
    if (test_hpps_wdt())
        panic("HPPS WDT test");
#endif // TEST_HPPS_WDT_STANDALONE

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

#if TEST_SRAM
    printf("[%u] looking for SRAM ...\r\n");
    file_load_from_sram();
#endif // TEST_SRAM

#if SERVER
    struct endpoint *endpoint;
#endif // SERVER

#if TEST_HPPS_TRCH_MAILBOX_SSW || TEST_HPPS_TRCH_MAILBOX
#define HPPS_RCV_IRQ_IDX MBOX_HPPS_TRCH__TRCH_RCV_INT
#define HPPS_ACK_IRQ_IDX MBOX_HPPS_TRCH__TRCH_ACK_INT
    struct irq *hpps_rcv_irq =
        nvic_request(MBOX_HPPS_TRCH__IRQ_START + HPPS_RCV_IRQ_IDX);
    struct irq *hpps_ack_irq =
        nvic_request(MBOX_HPPS_TRCH__IRQ_START + HPPS_ACK_IRQ_IDX);

     endpoint = &endpoints[ENDPOINT_HPPS];
     endpoint->rcv_irq = hpps_rcv_irq;
     endpoint->rcv_int_idx = HPPS_RCV_IRQ_IDX;
     endpoint->ack_irq = hpps_ack_irq;
     endpoint->rcv_int_idx = HPPS_ACK_IRQ_IDX;
#endif // TEST_HPPS_TRCH_MAILBOX_SSW || TEST_HPPS_TRCH_MAILBOX

#if TEST_HPPS_TRCH_MAILBOX_SSW
    struct mbox_link *hpps_link_ssw = mbox_link_connect(MBOX_HPPS_TRCH__BASE,
                    MBOX_HPPS_TRCH__HPPS_TRCH_SSW, MBOX_HPPS_TRCH__TRCH_HPPS_SSW,
                    hpps_rcv_irq, HPPS_RCV_IRQ_IDX,
                    hpps_ack_irq, HPPS_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link_ssw)
        panic("HPPS link SSW");

    // Never release the link, because we listen on it in main loop
#endif

#if TEST_HPPS_TRCH_MAILBOX
    struct mbox_link *hpps_link = mbox_link_connect( MBOX_HPPS_TRCH__BASE,
                    MBOX_HPPS_TRCH__HPPS_TRCH, MBOX_HPPS_TRCH__TRCH_HPPS,
                    hpps_rcv_irq, HPPS_RCV_IRQ_IDX,
                    hpps_ack_irq, HPPS_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        panic("HPPS link");

    // Never release the link, because we listen on it in main loop
#endif

#if TEST_RTPS_TRCH_MAILBOX
#define LSIO_RCV_IRQ_IDX MBOX_LSIO__TRCH_RCV_INT
#define LSIO_ACK_IRQ_IDX MBOX_LSIO__TRCH_ACK_INT
     struct irq *lsio_rcv_irq = nvic_request(MBOX_LSIO__IRQ_START + LSIO_RCV_IRQ_IDX);
     struct irq *lsio_ack_irq = nvic_request(MBOX_LSIO__IRQ_START + LSIO_ACK_IRQ_IDX);

     endpoint = &endpoints[ENDPOINT_LSIO];
     endpoint->rcv_irq = lsio_rcv_irq;
     endpoint->rcv_int_idx = LSIO_RCV_IRQ_IDX;
     endpoint->ack_irq = lsio_rcv_irq;
     endpoint->ack_int_idx = LSIO_ACK_IRQ_IDX;

    struct mbox_link *rtps_link = mbox_link_connect(MBOX_LSIO__BASE,
                    MBOX_LSIO__RTPS_TRCH, MBOX_LSIO__TRCH_RTPS,
                    lsio_rcv_irq, LSIO_RCV_IRQ_IDX,
                    lsio_ack_irq, LSIO_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        panic("RTPS link");

    // Never disconnect the link, because we listen on it in main loop
#endif // TEST_RTPS_TRCH_MAILBOX

#if TEST_RTPS_WDT
    watchdog_rtps_start();
#endif // TEST_RTPS_WDT

#if TEST_HPPS_WDT
    watchdog_hpps_start();
#endif // TEST_HPPS_WDT

#if TEST_BOOT_RTPS
    reset_component(COMPONENT_RTPS);
#endif // TEST_BOOT_RTPS

#if TEST_BOOT_HPPS
    reset_component(COMPONENT_HPPS);
#endif // TEST_BOOT_HPPS

#if TEST_IPI
    printf("Testing IPI...\r\n");
    // uint32_t * addr = (unsigned int *) 0xff310000; /* IPI_TRIG */
    uint32_t val = (1 << 8); 	/* RPU0 */
    * ((uint32_t *) 0xff300000)= val;
    * ((uint32_t *) 0xff310000)= val;
    * ((uint32_t *) 0xff320000)= val;
    * ((uint32_t *) 0xff380000)= val;
    printf("M4: after trigger interrupt to R52\r\n");
#endif // TEST_IPI

#if SERVER
    server_init(endpoints, sizeof(endpoints) / sizeof(endpoints[0]));
#endif // SERVER

#if TEST_TRCH_WDT
    watchdog_trch_start();
#endif // TEST_RTPS_WDT


    unsigned iter = 0; // just to make output that changes to see it
    while (1) {
        printf("TRCH: main loop\r\n");

#if TEST_TRCH_WDT
        // Kicking from here is insufficient, because we sleep. There are two
        // ways to complete: (A) have TRCH disable the watchdog in response to
        // the WFI output signal from the core, and/or (B) have a scheduler
        // (with a tick interval shorter than the watchdog timeout interval)
        // and kick from the scheuduler tick. As a temporary stop-gap, we go
        // with (C): kick on return from WFI/WFI as a result of first stage
        // timeout IRQ.
        watchdog_trch_kick();
#endif // TEST_TRCH_WDT

        //printf("main\r\n");

#if SERVER
        struct cmd cmd;
        while (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // SERVER


        printf("[%u] Waiting for interrupt...\r\n", ++iter);
        asm("wfi");
    }
}
