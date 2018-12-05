#include "printf.h"
#include "uart.h"
#include "float.h"
#include "mailbox.h"
#include "command.h"
#include "hwinfo.h"
#include "panic.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "dma.h"
#include "intc.h"
#include "gic.h"
#include "gtimer.h"
#include "watchdog.h"
#include "test.h"

extern unsigned char _text_start;
extern unsigned char _text_end;
extern unsigned char _data_start;
extern unsigned char _data_end;
extern unsigned char _bss_start;
extern unsigned char _bss_end;

extern void enable_caches(void);
extern void compare_sorts(void);

void enable_interrupts (void)
{
	unsigned long temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "bic %0, %0, #0x80\n"
			     "msr cpsr_c, %0"
			     : "=r" (temp)
			     :
			     : "memory");
}

void soft_reset (void) 
{
	unsigned long temp;
	__asm__ __volatile__("mov r1, #2\n"
			     "mcr p15, 4, r1, c12, c0, 2\n"); 
}

int main(void)
{
    cdns_uart_startup();
    printf("RTPS\r\n");

    enable_caches();
    enable_interrupts();

    gic_init((volatile uint32_t *)RTPS_GIC_BASE);

#if TEST_WDT_STANDALONE
    if (test_wdt())
        panic("wdt test");
#endif // TEST_WDT_STANDALONE

#if TEST_FLOAT
    if (test_float())
        panic("float test");
#endif // TEST_FLOAT

#if TEST_SORT
    if (test_sort())
        panic("sort test");
#endif // TEST_SORT

#if TEST_GTIMER
    if (test_gtimer())
        panic("gtimer test");
#endif // TEST_GTIMER

#if TEST_RT_MMU
    if (test_rt_mmu())
        panic("TRCH/RTPS->HPPS MMU test");
#endif // TEST_RT_MMU

#if TEST_RTPS_MMU
    if (test_rtps_mmu())
        panic("RTPS MMU test");
#endif // TEST_RT_MMU

#if TEST_RTPS_DMA
    if (test_rtps_dma())
        panic("RTPS DMA test");
#endif // TEST_RTPS_DMA

#if TEST_RTPS_TRCH_MAILBOX
    if (test_rtps_trch_mailbox())
        panic("RTPS->TRCH mailbox");
#endif // TEST_RTPS_TRCH_MAILBOX

#if TEST_HPPS_RTPS_MAILBOX
#define HPPS_RCV_IRQ_IDX  MBOX_HPPS_RTPS__RTPS_RCV_INT
#define HPPS_ACK_IRQ_IDX  MBOX_HPPS_RTPS__RTPS_ACK_INT
    struct irq *hpps_rcv_irq =
        gic_request(RTPS_IRQ__HR_MBOX_0 + HPPS_RCV_IRQ_IDX,
                    GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);
    struct irq *hpps_ack_irq =
        gic_request(RTPS_IRQ__HR_MBOX_0 + HPPS_ACK_IRQ_IDX,
                    GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);

    struct mbox_link *hpps_link = mbox_link_connect(MBOX_HPPS_RTPS__BASE,
                    MBOX_HPPS_RTPS__HPPS_RTPS, MBOX_HPPS_RTPS__RTPS_HPPS,
                    hpps_rcv_irq, HPPS_RCV_IRQ_IDX,
                    hpps_ack_irq, HPPS_ACK_IRQ_IDX,
                    /* server */ MASTER_ID_RTPS_CPU0,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        panic("HPPS link");
    // Never release the link, because we listen on it in main loop
#endif // TEST_HPPS_RTPS_MAILBOX

#if TEST_SOFT_RESET
    printf("Resetting...\r\n");
    /* this will generate "Undefined Instruction exception because HRMR is accessible only at EL2 */
    soft_reset();
    printf("ERROR: reached unrechable code: soft reset failed\r\n");
#endif // TEST_SOFT_RESET

#if TEST_WDT
    watchdog_init();
#endif // TEST_WDT

    unsigned iter = 0; // just to make output that changes to see it
    while (1) {
        printf("RTPS: main loop\r\n");

#if TEST_WDT
        // Kicking from here is insufficient, because we sleep. There are two
        // ways to complete: (A) have TRCH disable the watchdog in response to
        // the WFI output signal from the core, and/or (B) have a scheduler
        // (with a tick interval shorter than the watchdog timeout interval)
        // and kick from the scheuduler tick. As a temporary stop-gap, we go
        // with (C): kick on return from WFI/WFI as a result of first stage
        // timeout IRQ.
        watchdog_kick();
#endif // TEST_WDT

#if TEST_HPPS_RTPS_MAILBOX
        struct cmd cmd;
        while (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // TEST_HPPS_RTPS_MAILBOX

        printf("[%u] Waiting for interrupt...\r\n", iter++);
        asm("wfi");
    }
    
    return 0;
}

void irq_handler(unsigned intid) {
    printf("INTID #%u\r\n", intid);
    if (intid < GIC_NR_SGIS) { // SGI
        unsigned sgi = intid;
        switch (sgi) {
            default:
                printf("WARN: no ISR for SGI IRQ #%u\r\n", sgi);
        }
    } else if (intid < GIC_INTERNAL) { // PPI
        unsigned ppi = intid - GIC_NR_SGIS;
        switch (ppi) {
#if TEST_GTIMER
            case PPI_IRQ__TIMER_HYP:
                    gtimer_isr(GTIMER_HYP);
                    break;
            case PPI_IRQ__TIMER_PHYS:
                    gtimer_isr(GTIMER_PHYS);
                    break;
            case PPI_IRQ__TIMER_VIRT:
                    gtimer_isr(GTIMER_VIRT);
                    break;
#endif // TEST_GTIMER
#if TEST_WDT
            case PPI_IRQ__WDT:
                wdt_isr(wdt, /* stage */ 0);
                break;
#endif // TEST_WDT
            default:
                printf("WARN: no ISR for PPI IRQ #%u\r\n", ppi);
        }
    } else { // SPI
        unsigned irq = intid - GIC_INTERNAL;
        printf("IRQ #%u\r\n", irq);
        switch (irq) {
            // Only register the ISRs for mailbox ints that are used (see mailbox-map.h)
            // NOTE: we multiplex all mboxes (in one IP block) onto one pair of IRQs
    #if TEST_HPPS_RTPS_MAILBOX
            case RTPS_IRQ__HR_MBOX_0 + MBOX_HPPS_RTPS__RTPS_RCV_INT:
                    mbox_rcv_isr(MBOX_HPPS_RTPS__RTPS_RCV_INT);
                    break;
            case RTPS_IRQ__HR_MBOX_0 + MBOX_HPPS_RTPS__RTPS_ACK_INT:
                    mbox_ack_isr(MBOX_HPPS_RTPS__RTPS_ACK_INT);
                    break;
    #endif // TEST_HPPS_RTPS_MAILBOX
    #if TEST_RTPS_TRCH_MAILBOX
            case RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_RCV_INT:
                    mbox_rcv_isr(MBOX_LSIO__RTPS_RCV_INT);
                    break;
            case RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_ACK_INT:
                    mbox_ack_isr(MBOX_LSIO__RTPS_ACK_INT);
                    break;
    #endif // TEST_RTPS_TRCH_MAILBOX
    #if TEST_RTPS_DMA
            case RTPS_IRQ__RTPS_DMA_ABORT:
                    dma_abort_isr(rtps_dma);
                    break;
            case RTPS_IRQ__RTPS_DMA_EV0:
                    dma_event_isr(rtps_dma, 0);
                    break;
    #endif
            default:
                    printf("WARN: no ISR for IRQ #%u\r\n", irq);
        }
    }
}
