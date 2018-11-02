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
#include "hwinfo.h"
#include "intc.h"

#define TEST_RTPS
#define TEST_HPPS

#define TEST_FLOAT
#define TEST_HPPS_TRCH_MAILBOX_SSW
#define TEST_HPPS_TRCH_MAILBOX
#define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI
#define TEST_RTPS_TRCH_MMU
#define TEST_RTPS_TRCH_MMU_ACCESS // if not defined, then just init and deinit

// Page table should be in memory that is on a bus
// accessible from the MMUs master port ('dma' prop
// in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging
// from high-level Chiplet diagram.
#define RTPS_HPPS_PT_ADDR 0x8e000000
#define RTPS_HPPS_PT_SIZE   0x200000

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

#ifdef TEST_RTPS_TRCH_MMU

// A macro for testing convenience. Regions don't have to be the same.
#define MMU_TEST_REGION_SIZE 0x10000

    struct mmu *rt_mmu = mmu_create("RTPS/TRCH->HPPS",
		RTPS_TRCH_TO_HPPS_SMMU_BASE);
    if (!rt_mmu)
	panic("MMU init");

    // In this test, we share one allocator for all contexts
    struct balloc *ba = balloc_create("RT",
		(uint64_t *)RTPS_HPPS_PT_ADDR, RTPS_HPPS_PT_SIZE);

    struct mmu_context *rtps_ctx = mmu_context_create(rt_mmu, ba, MMU_PAGESIZE_4KB);
    if (!rtps_ctx)
	panic("MMU RTPS ctx");

    if (mmu_map(rtps_ctx, 0x8e100000,  0x8e100000, MMU_TEST_REGION_SIZE))
	panic("MMU RTPS identity mapping");
    if (mmu_map(rtps_ctx, 0xc0000000, 0x100000000, MMU_TEST_REGION_SIZE))
	panic("MMU RTPS window mapping");
    if (mmu_map(rtps_ctx, (uint32_t)HPPS_MBOX_BASE,
			  (uint32_t)HPPS_MBOX_BASE, HPSC_MBOX_AS_SIZE))
	panic("MMU RTPS mbox mapping");

    struct mmu_stream *rtps_stream =
		mmu_stream_create(MASTER_ID_RTPS_CPU0, rtps_ctx);
    if (!rtps_stream)
	panic("MMU RTPS stream");

    struct mmu_context *trch_ctx = mmu_context_create(rt_mmu, ba, MMU_PAGESIZE_4KB);
    if (!trch_ctx)
	panic("MMU TRCH ctx");

    if (mmu_map(trch_ctx, 0xc0000000, 0x100010000, MMU_TEST_REGION_SIZE))
	panic("MMU TRCH window mapping");
    if (mmu_map(trch_ctx, 0xc1000000, 0x100000000, MMU_TEST_REGION_SIZE))
	panic("MMU TRCH window mapping");
    if (mmu_map(trch_ctx, (uint32_t)HPPS_MBOX_BASE,
			  (uint32_t)HPPS_MBOX_BASE, HPSC_MBOX_AS_SIZE))
	panic("MMU TRCH mbox mapping");

    struct mmu_stream *trch_stream =
		mmu_stream_create(MASTER_ID_TRCH_CPU, trch_ctx);
    if (!trch_stream)
	panic("MMU TRCH stream");

    // In an alternative test, both streams could point to the same context

    mmu_enable(rt_mmu);

#ifdef TEST_RTPS_TRCH_MMU_ACCESS
    // In this test, TRCH accesses same location as RTPS but via a different virtual addr.
    // RTPS should read 0xc0000000 and find 0xbeeff00d, not 0xf00dbeef.

    volatile uint32_t *addr = (volatile uint32_t *)0xc1000000;
    uint32_t val = 0xbeeff00d;
    printf("%p <- %08x\r\n", addr, val);
    *addr = val;

    addr = (volatile uint32_t *)0xc0000000;
    val = 0xf00dbeef;
    printf("%p <- %08x\r\n", addr, val);
    *addr = val;
#else // !TEST_RTPS_TRCH_MMU_ACCESS

   // We can't always tear down, because we need to leave the MMU
   // configured for the other subsystems to do their test accesses

    mmu_disable(rt_mmu);

    if (mmu_stream_destroy(rtps_stream))
	panic("MMU RTPS stream destroy");

    if (mmu_unmap(rtps_ctx, (uint32_t)HPPS_MBOX_BASE, HPSC_MBOX_AS_SIZE))
	panic("MMU RTPS mbox unmapping");
    if (mmu_unmap(rtps_ctx, 0xc0000000, MMU_TEST_REGION_SIZE))
	panic("MMU RTPS window unmapping");
    if (mmu_unmap(rtps_ctx, 0x8e100000,  MMU_TEST_REGION_SIZE))
	panic("MMU RTPS identity unmapping");

    if (mmu_context_destroy(rtps_ctx))
	panic("MMU RTPS ctx destroy");

    if (mmu_stream_destroy(trch_stream))
	panic("MMU TRCH stream destrtoy");

    if (mmu_unmap(trch_ctx, (uint32_t)HPPS_MBOX_BASE, HPSC_MBOX_AS_SIZE))
	panic("MMU TRCH mbox unmapping");
    if (mmu_unmap(trch_ctx, 0xc1000000, MMU_TEST_REGION_SIZE))
	panic("MMU TRCH window unmapping");
    if (mmu_unmap(trch_ctx, 0xc0000000, MMU_TEST_REGION_SIZE))
	panic("MMU TRCH window unmapping");

    if (mmu_context_destroy(trch_ctx))
	panic("MMU TRCH ctx destroy");

    if (mmu_destroy(rt_mmu))
    balloc_destroy(ba);
#endif // !TEST_RTPS_TRCH_MMU_ACCESS
#endif // TEST_RTPS_TRCH_MMU

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
