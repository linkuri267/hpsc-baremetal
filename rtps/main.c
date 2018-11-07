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
//    asm(".global __use_hlt_semihosting");
    cdns_uart_startup(); 	// init UART
    printf("R52 is alive\r\n");


    /* Display a welcome message via semihosting */
    printf("Cortex-R52 bare-metal startup example\r\n");

    /* Enable the caches */
    enable_caches();

    /* Enable GIC */
/*    printf("start of arm_gic_setup()\n");
    arm_gic_setup();
    printf("end of arm_gic_setup()\n");
*/
    enable_interrupts();

    intc_init((volatile uint32_t *)RTPS_GIC_BASE);

#if TEST_FLOAT
    if (test_float())
        panic("float test");
#endif // TEST_FLOAT

#if TEST_SORT
    if (test_sort())
        panic("sort test");
#endif // TEST_SORT

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
    struct mbox_link *hpps_link = mbox_link_connect(
                    HPPS_MBOX_BASE, HPPS_MBOX_IRQ_START,
                    MBOX_HPPS_HPPS_RTPS, MBOX_HPPS_RTPS_HPPS, 
                    MBOX_HPPS_RTPS_RCV_INT, MBOX_HPPS_RTPS_ACK_INT,
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

    while (1) {

#if TEST_HPPS_RTPS_MAILBOX
        struct cmd cmd;
        while (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // TEST_HPPS_RTPS_MAILBOX

        printf("Waiting for interrupt...\r\n");
        asm("wfi");
    }
    
    return 0;
}

void irq_handler(unsigned irq) {
    printf("IRQ #%u\r\n", irq);
    switch (irq) {
        // Only register the ISRs for mailbox ints that are used (see mailbox-map.h)
        // NOTE: we multiplex all mboxes (in one IP block) onto one pair of IRQs
#if TEST_HPPS_RTPS_MAILBOX
        case HPPS_MBOX_IRQ_START + MBOX_HPPS_RTPS_RCV_INT:
                mbox_rcv_isr(MBOX_HPPS_RTPS_RCV_INT);
                break;
        case HPPS_MBOX_IRQ_START  + MBOX_HPPS_RTPS_ACK_INT:
                mbox_ack_isr(MBOX_HPPS_RTPS_ACK_INT);
                break;
#endif // TEST_HPPS_RTPS_MAILBOX
#if TEST_RTPS_TRCH_MAILBOX
        case LSIO_MBOX_IRQ_START + MBOX_LSIO_RTPS_RCV_INT:
                mbox_rcv_isr(MBOX_LSIO_RTPS_RCV_INT);
                break;
        case LSIO_MBOX_IRQ_START + MBOX_LSIO_RTPS_ACK_INT:
                mbox_ack_isr(MBOX_LSIO_RTPS_ACK_INT);
                break;
#endif // TEST_RTPS_TRCH_MAILBOX
#if TEST_RTPS_DMA
        case RTPS_DMA_ABORT_IRQ:
                dma_abort_isr(rtps_dma);
                break;
        case RTPS_DMA_EV0_IRQ:
                dma_event_isr(rtps_dma, 0);
                break;
#endif
        default:
                printf("WARN: no ISR for IRQ #%u\r\n", irq);
    }
}
