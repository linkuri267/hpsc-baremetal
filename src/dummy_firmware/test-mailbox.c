#include <stdbool.h>

#include "mailbox.h"
#include "command.h"
#include "printf.h"
#include "nvic.h"
#include "busid.h"
#include "panic.h"

struct cmd_ctx {
    struct mbox *reply_mbox;
    volatile bool reply_acked;
    const char *origin; // for pretty-print
};

#define MBOX_FROM_RTPS_INSTANCE 0
#define MBOX_TO_RTPS_INSTANCE 1

// Globals because used in ISRs
static struct mbox *mbox_from_rtps;
static struct mbox *mbox_to_rtps;

#define MBOX_FROM_HPPS_INSTANCE 0
#define MBOX_TO_HPPS_INSTANCE 1

// Globals because used in ISRs
static struct mbox *mbox_from_hpps;
static struct mbox *mbox_to_hpps;

static struct cmd_ctx rtps_cmd_ctx;
static struct cmd_ctx hpps_cmd_ctx;

static void handle_ack(void *arg)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)arg;
    printf("ACK from %s\r\n", ctx->origin);
    ctx->reply_acked = true;
}


static void handle_cmd(void *arg, uint32_t *msg, size_t size)
{
    struct cmd_ctx *ctx = (struct cmd_ctx *)arg;
    unsigned i;

    struct cmd cmd; // can't use initializer, because GCC inserts a memset for initing .arg
    cmd.reply_mbox = ctx->reply_mbox;
    cmd.reply_acked = &ctx->reply_acked;
    cmd.cmd = msg[0];
    for (i = 0; i < HPSC_MBOX_DATA_REGS - 1 && i < MAX_CMD_ARG_LEN; ++i)
        cmd.arg[i] = msg[1 + i];

    printf("CMD (%u, %u ...) from %s\r\n", cmd.cmd, cmd.arg[0], ctx->origin);

    if (cmd_enqueue(&cmd))
        panic("failed to enqueue command");
}

void setup_hpps_trch_mailbox() {
    printf("Testing HPPS-TRCH mailbox...\r\n");

    nvic_int_enable(HPPS_MAILBOX_IRQ_A(MBOX_FROM_HPPS_INSTANCE));

    mbox_from_hpps = mbox_claim_owner(HPPS_MBOX_BASE, MBOX_FROM_HPPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0);
    if (!mbox_from_hpps)
        panic("claim HPPS in mbox\r\n");

    nvic_int_enable(HPPS_MAILBOX_IRQ_B(MBOX_TO_HPPS_INSTANCE));

    mbox_to_hpps = mbox_claim_owner(HPPS_MBOX_BASE, MBOX_TO_HPPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_HPPS_CPU0);
    if (!mbox_to_hpps)
        panic("claim HPPS out mbox\r\n");

    hpps_cmd_ctx.origin = "HPPS";
    hpps_cmd_ctx.reply_mbox = mbox_to_hpps;
    hpps_cmd_ctx.reply_acked = false;

    if (mbox_init_out(mbox_to_hpps, handle_ack, &hpps_cmd_ctx))
        panic("init HPPS out mbox\r\n");
    if (mbox_init_in(mbox_from_hpps, handle_cmd, &hpps_cmd_ctx))
        panic("init HPPS in mbox\r\n");

    // Never release, just keep listening. Alternatively, could listen to one
    // request, then reply and release.
}

void setup_rtps_trch_mailbox() {
    printf("Testing RTPS-TRCH mailbox...\r\n");

    nvic_int_enable(LSIO_MAILBOX_IRQ_A(MBOX_FROM_RTPS_INSTANCE));

    mbox_from_rtps = mbox_claim_owner(LSIO_MBOX_BASE, MBOX_FROM_RTPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_from_rtps)
        panic("claim RTPS in mbox\r\n");

    nvic_int_enable(LSIO_MAILBOX_IRQ_B(MBOX_TO_RTPS_INSTANCE));

    mbox_to_rtps = mbox_claim_owner(LSIO_MBOX_BASE, MBOX_TO_RTPS_INSTANCE, MASTER_ID_TRCH_CPU, MASTER_ID_RTPS_CPU0);
    if (!mbox_to_rtps)
        panic("claim RTPS out mbox\r\n");

    rtps_cmd_ctx.origin = "RTPS";
    rtps_cmd_ctx.reply_mbox = mbox_to_rtps;
    rtps_cmd_ctx.reply_acked = false;

    if (mbox_init_out(mbox_to_rtps, handle_ack, &rtps_cmd_ctx))
        panic("init RTPS out mbox\r\n");
    if (mbox_init_in(mbox_from_rtps, handle_cmd, &rtps_cmd_ctx))
        panic("init RTPS in mbox\r\n");

    // Never release, just keep listening. Alternatively, could listen to one
    // request, then reply and release.
}

void mbox_rtps_rcv_isr()
{
    mbox_rcv_isr(mbox_from_rtps);
}
void mbox_rtps_ack_isr()
{
    mbox_ack_isr(mbox_to_rtps);
}

void mbox_hpps_rcv_isr()
{
    mbox_rcv_isr(mbox_from_hpps);
}
void mbox_hpps_ack_isr()
{
    mbox_ack_isr(mbox_to_hpps);
}
