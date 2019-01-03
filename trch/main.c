#define DEBUG 0

#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
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
#include "mmus.h"
#include "dmas.h"
#include "boot.h"
#include "smc.h"
#include "systick.h"
#include "sleep.h"
#include "arm.h"
#include "test.h"

#define SYSTICK_INTERVAL_MS     500
#define SYSTICK_INTERVAL_CYCLES (SYSTICK_INTERVAL_MS * (SYSTICK_CLK_HZ / 1000))
#define MAIN_LOOP_SILENT_ITERS 16

#define SERVER (CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX || CONFIG_RTPS_TRCH_MAILBOX)

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

#if CONFIG_SYSTICK
static void systick_tick(void *arg)
{
    DPRINTF("MAIN: sys tick\r\n");

#if CONFIG_TRCH_WDT
    // Note: we kick here in the ISR instead of relying on the main loop
    // wakeing up from WFE as a result of ISR, because the main loop might not
    // be sleeping but might be performing a long operation, in which case it
    // might not get to the kick statement at the beginning of the main loop in
    // time.
    watchdog_trch_kick();
#endif // CONFIG_TRCH_WDT

#if CONFIG_SLEEP_TIMER
    sleep_tick(SYSTICK_INTERVAL_CYCLES);
#endif // CONFIG_SLEEP_TIMER
}
#endif // CONFIG_SYSTICK

int main ( void )
{
    cdns_uart_startup(); // init UART peripheral
    printf("\r\n\r\nTRCH\r\n");

    printf("ENTER PRIVELEGED MODE: svc #0\r\n");
    asm("svc #0");

    nvic_init((volatile uint32_t *)TRCH_SCS_BASE);

    sleep_set_busyloop_factor(800000); // empirically calibrarted

#if TEST_SYSTICK
    if (test_systick())
        panic("TRCH systick test");
#endif // TEST_SYSTICK

#if CONFIG_SYSTICK
    systick_config(SYSTICK_INTERVAL_CYCLES, systick_tick, NULL);
    systick_enable();

#if CONFIG_SLEEP_TIMER
    sleep_set_clock(SYSTICK_CLK_HZ);
#endif // CONFIG_SLEEP_TIMER
#endif // CONFIG_SYSTICK

#if TEST_TRCH_WDT
    if (test_trch_wdt())
        panic("TRCH WDT test");
#endif // TEST_TRCH_WDT

#if TEST_RTPS_WDT
    if (test_rtps_wdt())
        panic("RTPS WDT test");
#endif // TEST_RTPS_WDT

#if TEST_HPPS_WDT
    if (test_hpps_wdt())
        panic("HPPS WDT test");
#endif // TEST_HPPS_WDT

#if TEST_FLOAT
    if (test_float())
        panic("float test");
#endif // TEST_FLOAT

#if TEST_TRCH_DMA
    if (test_trch_dma())
        panic("TRCH DMA test");
#endif // TEST_TRCH_DMA

#if CONFIG_TRCH_DMA
    struct dma *trch_dma = trch_dma_init();
    if (!trch_dma)
        panic("TRCH DMA");
    // never destroy, it is used by drivers
#else // !CONFIG_TRCH_DMA
    struct dma *trch_dma = NULL;
#endif // !CONFIG_TRCH_DMA

    if (smc_init(trch_dma))
        panic("SMC init");

#if CONFIG_RT_MMU
    if (rt_mmu_init())
        panic("RTPS/TRCH-HPPS MMU setup");
    // Never de-init since need some of the mappings while running. We could
    // remove mappings for loading the boot image binaries, but we don't
    // bother, since then would have to recreate them when reseting HPPS/RTPS.
#endif // CONFIG_RT_MMU

#if TEST_RT_MMU
    if (test_rt_mmu())
        panic("RTPS/TRCH-HPPS MMU test");
#endif // TEST_RT_MMU

#if SERVER
    struct endpoint *endpoint;
#endif // SERVER

#if CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX
#define HPPS_RCV_IRQ_IDX MBOX_HPPS_TRCH__TRCH_RCV_INT
#define HPPS_ACK_IRQ_IDX MBOX_HPPS_TRCH__TRCH_ACK_INT
    struct irq *hpps_rcv_irq =
        nvic_request(TRCH_IRQ__HT_MBOX_0 + HPPS_RCV_IRQ_IDX);
    struct irq *hpps_ack_irq =
        nvic_request(TRCH_IRQ__HT_MBOX_0 + HPPS_ACK_IRQ_IDX);

     endpoint = &endpoints[ENDPOINT_HPPS];
     endpoint->rcv_irq = hpps_rcv_irq;
     endpoint->rcv_int_idx = HPPS_RCV_IRQ_IDX;
     endpoint->ack_irq = hpps_ack_irq;
     endpoint->ack_int_idx = HPPS_ACK_IRQ_IDX;
#endif // CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX

#if CONFIG_HPPS_TRCH_MAILBOX_SSW
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

#if CONFIG_HPPS_TRCH_MAILBOX
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

#if CONFIG_RTPS_TRCH_MAILBOX
#define LSIO_RCV_IRQ_IDX MBOX_LSIO__TRCH_RCV_INT
#define LSIO_ACK_IRQ_IDX MBOX_LSIO__TRCH_ACK_INT
     struct irq *lsio_rcv_irq = nvic_request(TRCH_IRQ__TR_MBOX_0 + LSIO_RCV_IRQ_IDX);
     struct irq *lsio_ack_irq = nvic_request(TRCH_IRQ__TR_MBOX_0 + LSIO_ACK_IRQ_IDX);

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
#endif // CONFIG_RTPS_TRCH_MAILBOX

#if CONFIG_RTPS_WDT
    watchdog_rtps_init();
#endif // CONFIG_RTPS_WDT

#if CONFIG_HPPS_WDT
    watchdog_hpps_init();
#endif // CONFIG_HPPS_WDT

    if (boot_config())
        panic("BOOT CFG");

#if CONFIG_BOOT_RTPS
    boot_request(SUBSYS_RTPS);
#endif // CONFIG_BOOT_RTPS

#if CONFIG_BOOT_HPPS
    boot_request(SUBSYS_HPPS);
#endif // CONFIG_BOOT_HPPS

#if SERVER
    server_init(endpoints, sizeof(endpoints) / sizeof(endpoints[0]));
    cmd_handler_register(server_process);
#endif // SERVER

#if CONFIG_TRCH_WDT
    watchdog_trch_start();
#endif // CONFIG_RTPS_WDT

    unsigned iter = 0;
    while (1) {
        bool verbose = iter++ % MAIN_LOOP_SILENT_ITERS == 0;
        if (verbose)
            printf("TRCH: main loop\r\n");

#if CONFIG_TRCH_WDT && !CONFIG_SYSTICK // with SysTick, we kick from ISR
        // Kicking from here is insufficient, because we sleep. There are two
        // ways to complete: (A) have TRCH disable the watchdog in response to
        // the WFI output signal from the core, and/or (B) have a scheduler
        // (with a tick interval shorter than the watchdog timeout interval)
        // and kick from the scheuduler tick. As a temporary stop-gap, we go
        // with (C): kick on return from WFI/WFI as a result of first stage
        // timeout IRQ.
        watchdog_trch_kick();
#endif // CONFIG_TRCH_WDT && !CONFIG_SYSTICK

        //printf("main\r\n");

        subsys_t subsys;
        while (!boot_handle(&subsys)) {
            boot_reboot(subsys);
            verbose = true; // to end log with 'waiting' msg
        }

#if SERVER
        struct cmd cmd;
        while (!cmd_dequeue(&cmd)) {
            cmd_handle(&cmd);
            verbose = true; // to end log with 'waiting' msg
        }
#endif // SERVER

        int_disable(); // the check and the WFI must be atomic
        if (!cmd_pending() && !boot_pending()) {
            if (verbose)
                printf("[%u] Waiting for interrupt...\r\n", iter);
            asm("wfi"); // ignores PRIMASK set by int_disable
        }
        int_enable();
    }
}
