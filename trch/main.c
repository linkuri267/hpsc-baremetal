#define DEBUG 0

#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "console.h"
#include "command.h"
#include "mmu.h"
#include "panic.h"
#include "llist.h"
#include "mem-map.h"
#include "mailbox-link.h"
#include "shmem-link.h"
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

#define SERVER_MAILBOXES (CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX || CONFIG_RTPS_TRCH_MAILBOX)
#define SERVER_SHMEM (CONFIG_HPPS_TRCH_SHMEM || CONFIG_HPPS_TRCH_SHMEM_SSW)
#define SERVER (SERVER_MAILBOXES || SERVER_SHMEM)

typedef enum {
    ENDPOINT_HPPS = 0,
    ENDPOINT_LSIO,
} endpoint_t;

// can't have empty array, so keep a NULL entry
struct endpoint endpoints[] = {
#if SERVER_MAILBOXES
    [ENDPOINT_HPPS] = { .base = MBOX_HPPS_TRCH__BASE },
    [ENDPOINT_LSIO] = { .base = MBOX_LSIO__BASE },
#endif // SERVER_MAILBOXES
    { 0 }
};
#define ENDPOINT_COUNT (sizeof(endpoints) - sizeof(struct endpoint) / sizeof(struct endpoint))

#if CONFIG_TRCH_WDT
static bool trch_wdt_started = false;
#endif // CONFIG_TRCH_WDT

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
    if (trch_wdt_started)
        watchdog_kick(COMP_CPU_TRCH);
#endif // CONFIG_TRCH_WDT

#if CONFIG_SLEEP_TIMER
    sleep_tick(SYSTICK_INTERVAL_CYCLES);
#endif // CONFIG_SLEEP_TIMER
}
#endif // CONFIG_SYSTICK

int main ( void )
{
    console_init();
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

#if TEST_ETIMER
    if (test_etimer())
        panic("Elapsed Timer test");
#endif // TEST_ETIMER

#if TEST_RTI_TIMER
    if (test_core_rti_timer())
        panic("RTI Timer test");
#endif // TEST_RTI_TIMER

#if TEST_WDTS
    if (test_wdts())
        panic("WDT test");
#endif // TEST_WDTS

#if TEST_FLOAT
    if (test_float())
        panic("float test");
#endif // TEST_FLOAT

#if TEST_TRCH_DMA
    if (test_trch_dma())
        panic("TRCH DMA test");
#endif // TEST_TRCH_DMA

#if TEST_SHMEM
    if (test_shmem())
        panic("shmem test");
#endif // TEST_SHMEM

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

#if CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX
#define HPPS_RCV_IRQ_IDX MBOX_HPPS_TRCH__TRCH_RCV_INT
#define HPPS_ACK_IRQ_IDX MBOX_HPPS_TRCH__TRCH_ACK_INT
    endpoints[ENDPOINT_HPPS].rcv_irq =
        nvic_request(TRCH_IRQ__HT_MBOX_0 + HPPS_RCV_IRQ_IDX);
    endpoints[ENDPOINT_HPPS].rcv_int_idx = HPPS_RCV_IRQ_IDX;
    endpoints[ENDPOINT_HPPS].ack_irq =
        nvic_request(TRCH_IRQ__HT_MBOX_0 + HPPS_ACK_IRQ_IDX);
    endpoints[ENDPOINT_HPPS].ack_int_idx = HPPS_ACK_IRQ_IDX;
#endif // CONFIG_HPPS_TRCH_MAILBOX_SSW || CONFIG_HPPS_TRCH_MAILBOX

#if CONFIG_HPPS_TRCH_MAILBOX_SSW
    struct link *hpps_link_ssw = mbox_link_connect("HPPS_MBOX_SSW_LINK",
                    MBOX_HPPS_TRCH__BASE,
                    MBOX_HPPS_TRCH__HPPS_TRCH_SSW, MBOX_HPPS_TRCH__TRCH_HPPS_SSW,
                    endpoints[ENDPOINT_HPPS].rcv_irq, HPPS_RCV_IRQ_IDX,
                    endpoints[ENDPOINT_HPPS].ack_irq, HPPS_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link_ssw)
        panic("HPPS_MBOX_SSW_LINK");

    // Never release the link, because we listen on it in main loop
#endif

#if CONFIG_HPPS_TRCH_MAILBOX
    struct link *hpps_link = mbox_link_connect("HPPS_MBOX_LINK",
                    MBOX_HPPS_TRCH__BASE,
                    MBOX_HPPS_TRCH__HPPS_TRCH, MBOX_HPPS_TRCH__TRCH_HPPS,
                    endpoints[ENDPOINT_HPPS].rcv_irq, HPPS_RCV_IRQ_IDX,
                    endpoints[ENDPOINT_HPPS].ack_irq, HPPS_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        panic("HPPS_MBOX_LINK");

    // Never release the link, because we listen on it in main loop
#endif

#if CONFIG_RTPS_TRCH_MAILBOX
#define LSIO_RCV_IRQ_IDX MBOX_LSIO__TRCH_RCV_INT
#define LSIO_ACK_IRQ_IDX MBOX_LSIO__TRCH_ACK_INT
    endpoints[ENDPOINT_LSIO].rcv_irq =
        nvic_request(TRCH_IRQ__TR_MBOX_0 + LSIO_RCV_IRQ_IDX);
    endpoints[ENDPOINT_LSIO].rcv_int_idx = LSIO_RCV_IRQ_IDX;
    endpoints[ENDPOINT_LSIO].ack_irq =
        nvic_request(TRCH_IRQ__TR_MBOX_0 + LSIO_ACK_IRQ_IDX);
    endpoints[ENDPOINT_LSIO].ack_int_idx = LSIO_ACK_IRQ_IDX;

    struct link *rtps_link = mbox_link_connect("RTPS_MBOX_LINK",
                    MBOX_LSIO__BASE,
                    MBOX_LSIO__RTPS_TRCH, MBOX_LSIO__TRCH_RTPS,
                    endpoints[ENDPOINT_LSIO].rcv_irq, LSIO_RCV_IRQ_IDX,
                    endpoints[ENDPOINT_LSIO].ack_irq, LSIO_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        panic("RTPS_MBOX_LINK");

    // Never disconnect the link, because we listen on it in main loop
#endif // CONFIG_RTPS_TRCH_MAILBOX

#if SERVER_SHMEM
    struct llist link_list;
    llist_init(&link_list);
#endif // SERVER_SHMEM

#if CONFIG_HPPS_TRCH_SHMEM
    struct link *hpps_link_shmem = shmem_link_connect("HPPS_SHMEM_LINK",
                    (void *)HPPS_SHM_ADDR__TRCH_HPPS,
                    (void *)HPPS_SHM_ADDR__HPPS_TRCH);
    if (!hpps_link_shmem)
        panic("HPPS_SHMEM_LINK");
    if (llist_insert(&link_list, hpps_link_shmem))
        panic("HPPS_SHMEM_LINK: llist_insert");

    // Never disconnect the link, because we listen on it in main loop
#endif // CONFIG_HPPS_TRCH_SHMEM

#if CONFIG_HPPS_TRCH_SHMEM_SSW
    struct link *hpps_link_shmem_ssw = shmem_link_connect("HPPS_SHMEM_SSW_LINK",
                    (void *)HPPS_SHM_ADDR__TRCH_HPPS_SSW,
                    (void *)HPPS_SHM_ADDR__HPPS_TRCH_SSW);
    if (!hpps_link_shmem_ssw)
        panic("HPPS_SHMEM_SSW_LINK");
    if (llist_insert(&link_list, hpps_link_shmem_ssw))
        panic("HPPS_SHMEM_SSW_LINK: llist_insert");

    // Never disconnect the link, because we listen on it in main loop
#endif // CONFIG_HPPS_TRCH_SHMEM_SSW

    if (boot_config())
        panic("BOOT CFG");

#if CONFIG_BOOT_RTPS_R52
    boot_request(SUBSYS_RTPS_R52);
#endif // CONFIG_BOOT_RTPS

#if CONFIG_BOOT_RTPS_A53
    boot_request(SUBSYS_RTPS_A53);
#endif // CONFIG_BOOT_RTPS_A53

#if CONFIG_BOOT_HPPS
    boot_request(SUBSYS_HPPS);
#endif // CONFIG_BOOT_HPPS

#if SERVER
    server_init(endpoints, ENDPOINT_COUNT);
    cmd_handler_register(server_process);
#endif // SERVER

#if CONFIG_TRCH_WDT
    watchdog_init_group(CPU_GROUP_TRCH);
    watchdog_start(COMP_CPU_TRCH);
    trch_wdt_started = true;
#endif // CONFIG_TRCH_WDT

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
        watchdog_kick(COMP_CPU_TRCH);
#endif // CONFIG_TRCH_WDT

        //printf("main\r\n");

        subsys_t subsys;
        while (!boot_handle(&subsys)) {
            boot_reboot(subsys);
            verbose = true; // to end log with 'waiting' msg
        }

#if SERVER
        struct cmd cmd;
#if SERVER_SHMEM
        struct link *link_curr;
        int sz;
        llist_iter_init(&link_list);
        do {
            link_curr = (struct link *) llist_iter_next(&link_list);
            if (!link_curr)
                break;
            sz = link_curr->recv(link_curr, cmd.msg, sizeof(cmd.msg));
            if (sz) {
                printf("%s: recv: got message\r\n", link_curr->name);
                cmd.link = link_curr;
                if (cmd_enqueue(&cmd))
                    panic("TRCH: failed to enqueue command");
            }
        } while (1);
#endif // SERVER_SHMEM
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
