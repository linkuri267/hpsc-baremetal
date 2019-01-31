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
    printf("%s: rcved ACK\r\n", link->name);
    mlink->cmd_ctx.tx_acked = true;
}

static void handle_cmd(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    int ret;
    struct cmd cmd; // can't use initializer, because GCC inserts a memset for initing .msg
    cmd.link = link;
    ASSERT(sizeof(cmd.msg) == HPSC_MBOX_DATA_SIZE); // o/w zero-fill rest of msg
    printf("%s: rcved cmd\r\n", link->name);
    ret = mbox_read(mlink->mbox_from, cmd.msg, sizeof(cmd.msg));
    if (ret <= 0) {
        printf("handle_cmd: failed to receive mailbox message!\r\n");
        return;
    }

    if (cmd_enqueue(&cmd))
        panic("failed to enqueue command");
}

static void handle_reply(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    printf("%s: rcved reply\r\n", link->name);
    int ret = mbox_read(mlink->mbox_from, mlink->cmd_ctx.reply,
                        mlink->cmd_ctx.reply_sz);
    if (ret <= 0) {
        printf("handle_reply: failed to receive mailbox message!\r\n");
        return;
    }
    mlink->cmd_ctx.reply_len = ret;
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
    mlink->cmd_ctx.tx_acked = false;
    return mbox_send(mlink->mbox_to, buf, sz);
}

static bool mbox_link_is_send_acked(struct link *link)
{
    struct mbox_link *mlink = link->priv;
    return mlink->cmd_ctx.tx_acked;
}

static int mbox_link_poll(struct link *link, int timeout_ms)
{
    // TODO: timeout
    struct mbox_link *mlink = link->priv;
    printf("mbox_link_poll: waiting for reply...\r\n");
    while (!mlink->cmd_ctx.reply_len);
    printf("mbox_link_poll: reply received\r\n");
    return mlink->cmd_ctx.reply_len * sizeof(uint32_t);
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
    if (!rc) {
        printf("mbox_link_request: send failed\r\n");
        return -1;
    }

    // TODO: timeout on ACKs as part of rtimeout_ms
    printf("mbox_link_request: waiting for ACK...\r\n");
    while (!mlink->cmd_ctx.tx_acked);
    printf("mbox_link_request: ACK received\r\n");

    rc = mbox_link_poll(link, rtimeout_ms);
    if (!rc)
        printf("mbox_link_request: recv failed\r\n");
    return rc;
}

struct link *mbox_link_connect(
        const char *name,
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
    link->name = name;
    link->disconnect = mbox_link_disconnect;
    link->send = mbox_link_send;
    link->is_send_acked = mbox_link_is_send_acked;
    link->request = mbox_link_request;
    link->recv = NULL;
    return link;

free_from:
    mbox_release(mlink->mbox_from);
free_links:
    mlink_free(mlink);
free_link:
    OBJECT_FREE(link);
    return NULL;
}
