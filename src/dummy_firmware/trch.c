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
// #define TEST_HPPS

// #define TEST_FLOAT
// #define TEST_HPPS_TRCH_MAILBOX
#define TEST_RTPS_TRCH_MAILBOX
// #define TEST_IPI
// #define TEST_RTPS_HPPS_MMU

struct cmd_ctx {
    struct mbox *reply_mbox;
    volatile bool reply_acked;
};

static void panic(const char *msg)
{
    printf("PANIC HALT: %s\r\n", msg);
    while (1); // halt
}

static void handle_ack_from_rtps(void *cbarg)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)cbarg;
    printf("received ACK from RTPS\r\n");
    ctx->reply_acked = true;
}


void handle_cmd_from_rtps(void *cbarg, uint32_t *msg, size_t size)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)cbarg;

    struct cmd cmd = { .cmd = msg[0], .arg = msg[1],
                       .reply_mbox = ctx->reply_mbox,
                       .reply_acked = &ctx->reply_acked };
    if (cmd_enqueue(&cmd))
        panic("failed to enqueue command");
}

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

    nvic_int_enable(HPPS_TRCH_MAILBOX_IRQ_A);

    struct mbox *hpps_mbox = mbox_claim_owner(HPPS_TRCH_MBOX_BASE, /* instance */ 0, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0)
    if (!hpps_mbox)
        panic("failed to claim HPPS->TRCH mailbox\r\n");
    if (mbox_init_in(hpps_mbox, cmd_handle, NULL))
        panic("failed to init HPPS->TRCH mailbox for incomming\r\n");
#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    printf("Testing RTPS-TRCH mailbox...\r\n");

    nvic_int_enable(RTPS_TRCH_MAILBOX_IRQ_A);

    struct mbox *mbox_from_rtps = mbox_claim_owner(RTPS_TRCH_MBOX_BASE, /* instance */ 0, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_from_rtps)
        panic("failed to claim RTPS->TRCH mailbox\r\n");

    nvic_int_enable(RTPS_TRCH_MAILBOX_IRQ_B);

    struct mbox *mbox_to_rtps = mbox_claim_owner(RTPS_TRCH_MBOX_BASE, /* instance */ 1, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_to_rtps)
        panic("failed to claim RTPS->TRCH mailbox\r\n");

    struct cmd_ctx rtps_cmd_ctx = {
        .reply_mbox = mbox_to_rtps,
        .reply_acked = false
    };
    if (mbox_init_out(mbox_to_rtps, handle_ack_from_rtps, &rtps_cmd_ctx))
        panic("failed to init RTPS->TRCH mailbox for outgoing\r\n");
    if (mbox_init_in(mbox_from_rtps, handle_cmd_from_rtps, &rtps_cmd_ctx))
        panic("failed to init RTPS->TRCH mailbox for incomming\r\n");

    // Never release, just keep listening. Alternatively, could listen to one
    // request, then reply and release.

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

#ifdef TEST_RTPS_TRCH_MAILBOX
        struct cmd cmd;
        if (!cmd_dequeue(&cmd))
            cmd_handle(&cmd);
#endif // endif

        // TODO: need to disable the interrupts between check and wfi, and
        // re-enable them atomically, otherwise will sleep with commands
        // in the queue.
        printf("Waiting for interrupt...\r\n");
        asm("wfi");
    }
}

void mbox_rtps_rcv_isr()
{
    mbox_rcv_isr(RTPS_TRCH_MBOX_BASE);
}
void mbox_rtps_ack_isr()
{
    mbox_ack_isr(RTPS_TRCH_MBOX_BASE);
}

void mbox_hpps_isr()
{
    mbox_rcv_isr(HPPS_TRCH_MBOX_BASE);
}
