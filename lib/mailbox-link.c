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
    size_t reply_sz_read;
};

struct mbox_link {
    struct object obj;
    unsigned idx_to;
    unsigned idx_from;
    struct mbox *mbox_from;
    struct mbox *mbox_to;
    struct cmd_ctx cmd_ctx;
};

static struct mbox_link_dev *devs[MBOX_DEV_COUNT] = {0};
static struct link links[MAX_LINKS] = {0};
static struct mbox_link mlinks[MAX_LINKS] = {0};

int mbox_link_dev_add(mbox_dev_id id, struct mbox_link_dev *dev)
{
    ASSERT(id < MBOX_DEV_COUNT);
    if (devs[id]) {
        printf("ERROR: mbox_link_dev_add: already added id=%u\r\n", id);
        return -1;
    }
    devs[id] = dev;
    return 0;
}

void mbox_link_dev_remove(mbox_dev_id id)
{
    ASSERT(id < MBOX_DEV_COUNT);
    devs[id] = NULL;
}

struct mbox_link_dev *mbox_link_dev_get(mbox_dev_id id)
{
    ASSERT(id < MBOX_DEV_COUNT);
    return devs[id];
}

static void handle_ack(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    printf("%s: handle_ack\r\n", link->name);
    mlink->cmd_ctx.tx_acked = true;
}

static void handle_cmd(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    struct cmd cmd; // can't use initializer, because GCC inserts a memset for initing .msg
    cmd.link = link;
    ASSERT(sizeof(cmd.msg) == HPSC_MBOX_DATA_SIZE); // o/w zero-fill rest of msg

    printf("%s: handle_cmd\r\n", link->name);
    // read never fails if sizeof(cmd.msg) > 0
    mbox_read(mlink->mbox_from, cmd.msg, sizeof(cmd.msg));
    if (cmd_enqueue(&cmd))
        panic("handle_cmd: failed to enqueue command");
}

static void handle_reply(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    printf("%s: handle_reply\r\n", link->name);
    mlink->cmd_ctx.reply_sz_read = mbox_read(mlink->mbox_from,
                                             mlink->cmd_ctx.reply,
                                             mlink->cmd_ctx.reply_sz);
}

static int mbox_link_disconnect(struct link *link) {
    struct mbox_link *mlink = link->priv;
    int rc;
    printf("%s: disconnect\r\n", link->name);
    // in case of failure, keep going and fwd code
    rc = mbox_release(mlink->mbox_from);
    rc |= mbox_release(mlink->mbox_to);
    OBJECT_FREE(mlink);
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
    printf("%s: poll: waiting for reply...\r\n", link->name);
    while (!mlink->cmd_ctx.reply_sz_read);
    printf("%s: poll: reply received\r\n", link->name);
    return mlink->cmd_ctx.reply_sz_read;
}

static int mbox_link_request(struct link *link,
                             int wtimeout_ms, void *wbuf, size_t wsz,
                             int rtimeout_ms, void *rbuf, size_t rsz)
{
    struct mbox_link *mlink = link->priv;
    int rc;

    printf("%s: request\r\n", link->name);
    mlink->cmd_ctx.reply_sz_read = 0;
    mlink->cmd_ctx.reply = rbuf;
    mlink->cmd_ctx.reply_sz = rsz / sizeof(uint32_t);

    rc = mbox_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printf("mbox_link_request: send failed\r\n");
        return -1;
    }

    // TODO: timeout on ACKs as part of rtimeout_ms
    printf("%s: request: waiting for ACK...\r\n", link->name);
    while (!mlink->cmd_ctx.tx_acked);
    printf("%s: request: ACK received\r\n", link->name);

    if (mlink->cmd_ctx.reply_sz == 0) return 1;
    rc = mbox_link_poll(link, rtimeout_ms);
    if (!rc)
        printf("mbox_link_request: recv failed\r\n");
    return rc;
}

struct link *mbox_link_connect(const char *name, struct mbox_link_dev *ldev,
                               unsigned idx_from, unsigned idx_to,
                               unsigned server, unsigned client)
{
    struct mbox_link *mlink;
    struct link *link;
    printf("%s: connect\r\n", name);
    link = OBJECT_ALLOC(links);
    if (!link)
        return NULL;

    mlink = OBJECT_ALLOC(mlinks);
    if (!mlink) {
        printf("ERROR: mbox_link_connect: failed to allocate mlink state\r\n");
        goto free_link;
    }

    mlink->idx_from = idx_from;
    mlink->idx_to = idx_to;

    union mbox_cb rcv_cb = { .rcv_cb = server ? handle_cmd : handle_reply };
    mlink->mbox_from = mbox_claim(ldev->base, idx_from,
                                  ldev->rcv_irq, ldev->rcv_int_idx,
                                  server, client, server, MBOX_INCOMING,
                                  rcv_cb, link);
    if (!mlink->mbox_from) {
        printf("ERROR: mbox_link_connect: failed to claim mbox_from\r\n");
        goto free_links;
    }

    union mbox_cb ack_cb = { .ack_cb = handle_ack };
    mlink->mbox_to = mbox_claim(ldev->base, idx_to,
                                ldev->ack_irq, ldev->ack_int_idx,
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
    OBJECT_FREE(mlink);
free_link:
    OBJECT_FREE(link);
    return NULL;
}
