#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "uart.h"
#include "float.h"
#include "reset.h"
#include "command.h"
#include "mmu.h"
#include "panic.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "mailbox.h"
#include "busid.h"
#include "intc.h"

#define TEST_RTPS
#define TEST_HPPS

// #define TEST_FLOAT
#define TEST_HPPS_TRCH_MAILBOX_SSW
#define TEST_HPPS_TRCH_MAILBOX
#define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI
#define TEST_RTPS_HPPS_MMU

// Page table should be in memory that is on a bus
// accessible from the MMUs master port ('dma' prop
// in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging
// from high-level Chiplet diagram.
#define RTPS_HPPS_PT_ADDR 0x8e000000

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

#ifdef TEST_RTPS_HPPS_MMU
    if (mmu_init(RTPS_HPPS_PT_ADDR))
	panic("MMU init");
    if (mmu_map(0x8e050000,  0x8e050000, 0x10000))
	panic("MMU identity mapping");
    if (mmu_map(0xc0000000, 0x100000000, 0x10000))
	panic("MMU window mapping");

    if (mmu_map((uint32_t)HPPS_MBOX_BASE, (uint32_t)HPPS_MBOX_BASE, 0x10000))
	panic("MMU mbox mapping");
#endif // TEST_RTPS_HPPS_MMU

#ifdef TEST_HPPS_TRCH_MAILBOX_SSW
    struct mbox_link *hpps_link_ssw = mbox_link_connect(
                    HPPS_MBOX_BASE, HPPS_MBOX_IRQ_START,
                    MBOX_HPPS_HPPS_TRCH_SSW, MBOX_HPPS_TRCH_HPPS_SSW,
                    MBOX_HPPS_TRCH_RCV_INT, MBOX_HPPS_TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link_ssw)
        panic("HPPS link SSW");

    // Never release the link, because we listen on it in main loop
#endif

#ifdef TEST_HPPS_TRCH_MAILBOX
    struct mbox_link *hpps_link = mbox_link_connect(
                    HPPS_MBOX_BASE, HPPS_MBOX_IRQ_START,
                    MBOX_HPPS_HPPS_TRCH, MBOX_HPPS_TRCH_HPPS,
                    MBOX_HPPS_TRCH_RCV_INT, MBOX_HPPS_TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        panic("HPPS link");

    // Never release the link, because we listen on it in main loop
#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    struct mbox_link *rtps_link = mbox_link_connect(
                    LSIO_MBOX_BASE, LSIO_MBOX_IRQ_START,
                    MBOX_LSIO_RTPS_TRCH, MBOX_LSIO_TRCH_RTPS,
                    MBOX_LSIO_TRCH_RCV_INT, MBOX_LSIO_TRCH_ACK_INT,
                    /* server */ MASTER_ID_TRCH_CPU,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        panic("RTPS link");

    // Never disconnect the link, because we listen on it in main loop
#endif // TEST_RTPS_TRCH_MAILBOX

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
        while (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // endif

        printf("Waiting for interrupt...\r\n");
        asm("wfi");
    }
}
