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
    const char *origin; // for pretty-print
};

#ifdef TEST_RTPS_TRCH_MAILBOX

#define MBOX_FROM_RTPS_INSTANCE 0
#define MBOX_TO_RTPS_INSTANCE 1

static struct mbox *mbox_from_rtps;
static struct mbox *mbox_to_rtps;
#endif // TEST_RTPS_TRCH_MAILBOX

#ifdef TEST_HPPS_TRCH_MAILBOX

#define MBOX_FROM_HPPS_INSTANCE 0
#define MBOX_TO_HPPS_INSTANCE 1

static struct mbox *mbox_from_hpps;
static struct mbox *mbox_to_hpps;
#endif // TEST_HPPS_TRCH_MAILBOX

static void panic(const char *msg)
{
    printf("PANIC HALT: %s\r\n", msg);
    while (1); // halt
}

static void handle_ack(void *cbarg)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)cbarg;
    printf("ACK from %s\r\n", ctx->origin);
    ctx->reply_acked = true;
}


void handle_cmd(void *cbarg, uint32_t *msg, size_t size)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)cbarg;

    struct cmd cmd = { .cmd = msg[0], .arg = msg[1],
                       .reply_mbox = ctx->reply_mbox,
                       .reply_acked = &ctx->reply_acked };
    printf("CMD (%u, %u) from %s\r\n",
           cmd.cmd, cmd.arg, ctx->origin);

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

    nvic_int_enable(HPPS_MAILBOX_IRQ_A(MBOX_FROM_HPPS_INSTANCE));

    mbox_from_hpps = mbox_claim_owner(HPPS_TRCH_MBOX_BASE, MBOX_FROM_HPPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0);
    if (!mbox_from_hpps)
        panic("claim HPPS in mbox\r\n");

    nvic_int_enable(HPPS_MAILBOX_IRQ_B(MBOX_TO_HPPS_INSTANCE));

    mbox_to_hpps = mbox_claim_owner(HPPS_TRCH_MBOX_BASE, MBOX_TO_HPPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0);
    if (!mbox_to_hpps)
        panic("claim HPPS out mbox\r\n");

    struct cmd_ctx hpps_cmd_ctx = {
        .origin = "HPPS",
        .reply_mbox = mbox_to_hpps,
        .reply_acked = false
    };
    if (mbox_init_out(mbox_to_hpps, handle_ack, &hpps_cmd_ctx))
        panic("init HPPS out mbox\r\n");
    if (mbox_init_in(mbox_from_hpps, handle_cmd, &hpps_cmd_ctx))
        panic("init HPPS in mbox\r\n");

    // Never release, just keep listening. Alternatively, could listen to one
    // request, then reply and release.

#endif

#ifdef TEST_RTPS_TRCH_MAILBOX
    printf("Testing RTPS-TRCH mailbox...\r\n");

    nvic_int_enable(LSIO_MAILBOX_IRQ_A(MBOX_FROM_RTPS_INSTANCE));

    mbox_from_rtps = mbox_claim_owner(RTPS_TRCH_MBOX_BASE, MBOX_FROM_RTPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_from_rtps)
        panic("claim RTPS in mbox\r\n");

    nvic_int_enable(LSIO_MAILBOX_IRQ_B(MBOX_TO_RTPS_INSTANCE));

    mbox_to_rtps = mbox_claim_owner(RTPS_TRCH_MBOX_BASE, MBOX_TO_RTPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_to_rtps)
        panic("claim RTPS out mbox\r\n");

    struct cmd_ctx rtps_cmd_ctx = {
        .origin = "RTPS",
        .reply_mbox = mbox_to_rtps,
        .reply_acked = false
    };
    if (mbox_init_out(mbox_to_rtps, handle_ack, &rtps_cmd_ctx))
        panic("init RTPS out mbox\r\n");
    if (mbox_init_in(mbox_from_rtps, handle_cmd, &rtps_cmd_ctx))
        panic("init RTPS in mbox\r\n");

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

#if defined(TEST_RTPS_TRCH_MAILBOX) || defined(TEST_HPPS_TRCH_MAILBOX)
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
#ifdef TEST_RTPS_TRCH_MAILBOX
    mbox_rcv_isr(mbox_from_rtps);
#endif
}
void mbox_rtps_ack_isr()
{
#ifdef TEST_RTPS_TRCH_MAILBOX
    mbox_ack_isr(mbox_to_rtps);
#endif
}

void mbox_hpps_rcv_isr()
{
#ifdef TEST_HPPS_TRCH_MAILBOX
    mbox_rcv_isr(mbox_from_hpps);
#endif
}
void mbox_hpps_ack_isr()
{
#ifdef TEST_HPPS_TRCH_MAILBOX
    mbox_ack_isr(mbox_to_hpps);
#endif
}
