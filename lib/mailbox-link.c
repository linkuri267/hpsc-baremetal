#include <stdbool.h>

#include "command.h"
#include "link.h"
#include "mailbox.h"
#include "mailbox-link.h"
#include "object.h"
#include "panic.h"
#include "printf.h"


#define MAX_LINKS 8

struct cmd_ctx {
    volatile bool tx_acked;
    uint32_t *reply;
    size_t reply_sz;
    size_t reply_len;
};

struct mbox_link {
    bool valid; // is entry in the array full
    unsigned idx_to;
    unsigned idx_from;
    struct mbox *mbox_from;
    struct mbox *mbox_to;
    struct cmd_ctx cmd_ctx;
};

static struct link links[MAX_LINKS] = {0};
static struct mbox_link mlinks[MAX_LINKS] = {0};

static void handle_ack(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    printf("rcved ACK\r\n");
    mlink->cmd_ctx.tx_acked = true;
}

static void handle_cmd(void *arg, void *buf, size_t sz)
{
    struct link *link = arg;
    size_t i;
    size_t msg_size = sz / sizeof(uint32_t);
    ASSERT(sz % sizeof(uint32_t) == 0);

    struct cmd cmd; // can't use initializer, because GCC inserts a memset for initing .msg
    cmd.link = link;
    for (i = 0; i < HPSC_MBOX_DATA_REGS && i < msg_size; ++i)
        cmd.msg[i] = ((uint32_t *) buf)[i];

    printf("rcved CMD (%u, %u ...)\r\n", cmd.msg[0], cmd.msg[1]);

    if (cmd_enqueue(&cmd))
        panic("failed to enqueue command");
}

static void handle_reply(void *arg, void *buf, size_t sz)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    size_t i;
    size_t msg_size = sz / sizeof(uint32_t);
    ASSERT(sz % sizeof(uint32_t) == 0);

    for (i = 0; i < msg_size && i < mlink->cmd_ctx.reply_sz; ++i)
        mlink->cmd_ctx.reply[i] = ((uint32_t *) buf)[i];
    mlink->cmd_ctx.reply_len = i;
}

static void mlink_clear(struct mbox_link *mlink)
{
    // We don't have a libc, so no memset
    mlink->idx_to = 0;
    mlink->idx_from = 0;
    mlink->mbox_from = NULL;
    mlink->mbox_to = NULL;
}

static struct mbox_link *mlink_alloc()
{
    size_t i = 0;
    struct mbox_link *mlink;
    while (mlinks[i].valid && i < MAX_LINKS)
        ++i;
    if (i == MAX_LINKS)
        return NULL;
    mlink = &mlinks[i];
    mlink_clear(mlink);
    mlink->valid = true;
    return mlink;
}

static void mlink_free(struct mbox_link *mlink)
{
    mlink->valid = false;
    mlink_clear(mlink);
}

static int mbox_link_disconnect(struct link *link) {
    struct mbox_link *mlink = link->priv;
    // in case of failure, keep going and fwd code
    int rc = mbox_release(mlink->mbox_from);
    rc |= mbox_release(mlink->mbox_to);
    mlink_free(mlink);
    OBJECT_FREE(link);
    return rc;
}

static int mbox_link_send(struct link *link, int timeout_ms, void *buf,
                          size_t sz)
{
    // TODO: timeout
    struct mbox_link *mlink = link->priv;
    uint32_t *arg = buf;
    mlink->cmd_ctx.tx_acked = false;
    printf("mbox_link_send: cmd %x arg %x..\r\n", arg[0], arg[1]);
    return mbox_send(mlink->mbox_to, buf, sz);
}

static bool mbox_link_is_send_acked(struct link *link)
{
    struct mbox_link *mlink = link->priv;
    return mlink->cmd_ctx.tx_acked;
}

static int mbox_link_recv(struct link *link, int timeout_ms, void *buf,
                          size_t sz)
{
    // TODO: timeout
    struct mbox_link *mlink = link->priv;
    uint32_t *reply = buf;
    size_t i;
    ASSERT(sz % sizeof(uint32_t) == 0);

    printf("mbox_link_recv: waiting for reply...\r\n");
    while (!mlink->cmd_ctx.reply_len);
    printf("mbox_link_recv: reply received\r\n");

    printf("rcved REPLY: ");
    for (i = 0; i < mlink->cmd_ctx.reply_len; ++i)
        printf("%u ", reply[i]);
    printf("\r\n");
    return 0;
}

static int mbox_link_request(struct link *link,
                             int wtimeout_ms, void *wbuf, size_t wsz,
                             int rtimeout_ms, void *rbuf, size_t rsz)
{
    struct mbox_link *mlink = link->priv;
    int rc;

    mlink->cmd_ctx.reply_len = 0;
    mlink->cmd_ctx.reply = rbuf;
    mlink->cmd_ctx.reply_sz = rsz / sizeof(uint32_t);

    rc = mbox_link_send(link, wtimeout_ms, wbuf, wsz);
    if (rc) {
        printf("mbox_link_request: send failed: rc %d\r\n", rc);
        return rc;
    }

    // TODO: timeout on ACKs as part of rtimeout_ms
    printf("mbox_link_request: waiting for ACK...\r\n");
    while (!mlink->cmd_ctx.tx_acked);
    printf("mbox_link_request: ACK received\r\n");

    return mbox_link_recv(link, rtimeout_ms, rbuf, rsz);
}

struct link *mbox_link_connect(
        volatile uint32_t *base,
        unsigned idx_from, unsigned idx_to,
        struct irq *rcv_irq, unsigned rcv_int_idx, /* int index within IP block */
        struct irq *ack_irq, unsigned ack_int_idx,
        unsigned server, unsigned client)
{
    struct mbox_link *mlink;
    struct link *link = OBJECT_ALLOC(links);
    if (!link)
        return NULL;

    mlink = mlink_alloc();
    if (!mlink) {
        printf("ERROR: mbox_link_connect: failed to allocate mlink state\r\n");
        goto free_link;
    }

    mlink->idx_from = idx_from;
    mlink->idx_to = idx_to;

    union mbox_cb rcv_cb = { .rcv_cb = server ? handle_cmd : handle_reply };
    mlink->mbox_from = mbox_claim(base, idx_from, rcv_irq, rcv_int_idx,
                                  server, client, server, MBOX_INCOMING,
                                  rcv_cb, link);
    if (!mlink->mbox_from) {
        printf("ERROR: mbox_link_connect: failed to claim mbox_from\r\n");
        goto free_links;
    }

    union mbox_cb ack_cb = { .ack_cb = handle_ack };
    mlink->mbox_to = mbox_claim(base, idx_to, ack_irq, ack_int_idx,
                                server, server, client, MBOX_OUTGOING,
                                ack_cb, link);
    if (!mlink->mbox_to) {
        printf("ERROR: mbox_link_connect: failed to claim mbox_to\r\n");
        goto free_from;
    }

    mlink->cmd_ctx.tx_acked = false;
    mlink->cmd_ctx.reply = NULL;

    link->priv = mlink;
    link->disconnect = mbox_link_disconnect;
    link->send = mbox_link_send;
    link->is_send_acked = mbox_link_is_send_acked;
    // link->recv = mbox_link_recv;
    link->request = mbox_link_request;
    return link;

free_from:
    mbox_release(mlink->mbox_from);
free_links:
    mlink_free(mlink);
free_link:
    OBJECT_FREE(link);
    return NULL;
}
