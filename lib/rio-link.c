#define DEBUG 0

#include <stdint.h>
#include <stdbool.h>

#include "console.h"
#include "panic.h"
#include "oop.h"
#include "object.h"
#include "rio-ep.h"
#include "command.h"
#include "sleep.h"
#include "link.h"

#define REPLY_SIZE CMD_MSG_SZ
#define MIN_SLEEP_MS 500

struct cmd_ctx {
    uint8_t *reply;
    size_t reply_sz;
    bool reply_received;
};

struct rio_link {
    struct link link;
    struct rio_ep *ep;
    unsigned mbox_local;
    unsigned letter_local;
    rio_devid_t dest;
    unsigned mbox_remote;
    unsigned letter_remote;
    unsigned seg_size;
    struct cmd_ctx cmd_ctx;
};

#define MAX_LINKS 2
static struct rio_link links[MAX_LINKS] = { 0 };

static void handle_request(void *arg)
{
    struct rio_link *rlink = arg;
    int rc;
    rio_devid_t src = 0;
    uint64_t rcv_time = 0;
    struct cmd cmd;

    ASSERT(rlink);

    cmd.link = &rlink->link;
    unsigned msg_sz = sizeof(cmd.msg);
    rc = rio_ep_msg_recv(rlink->ep, rlink->mbox_local, rlink->letter_local,
                         &src, &rcv_time, cmd.msg, &msg_sz);
    if (rc) goto exit;
    if (src != rlink->dest) {
        printf("ERROR: RIO LINK: received req from unexpected source: "
               "0x%x != 0x%x\r\n", src, rlink->dest);
        rc = 1;
        goto exit;
    }
    cmd.len = msg_sz;
    if (cmd_enqueue(&cmd)) {
        printf("ERROR: failed to enqueue command");
        rc = 2;
        goto exit;
    }
exit: /* for now, make failure fatal to catch issues */
    if (rc)
        panic("RIO LINK: failure when handling incoming request");
}

static void handle_reply(void *arg)
{
    struct rio_link *rlink = arg;
    int rc;
    rio_devid_t src = 0;
    uint64_t rcv_time = 0;

    ASSERT(rlink);

    rc = rio_ep_msg_recv(rlink->ep, rlink->mbox_local, rlink->letter_local,
                         &src, &rcv_time,
                         rlink->cmd_ctx.reply, &rlink->cmd_ctx.reply_sz);
    if (rc) goto exit;
    if (src != rlink->dest) {
        printf("ERROR: RIO LINK: received reply from unexpected source: "
               "0x%x != 0x%x\r\n", src, rlink->dest);
        rc = 1;
        goto exit;
    }
    rlink->cmd_ctx.reply_received = true;
exit: /* for now, make failure fatal to catch issues */
    if (rc)
        panic("RIO LINK: failure when handling incoming reply");
}

/* TODO: factor into link.c */
static void msleep_and_dec(int *ms_rem)
{
    msleep(MIN_SLEEP_MS);
    // if < 0, timeout is infinite
    if (*ms_rem > 0)
        *ms_rem -= *ms_rem >= MIN_SLEEP_MS ? MIN_SLEEP_MS : *ms_rem;
}

static int rio_link_poll(struct link *link, int timeout_ms)
{
    struct rio_link *rlink = container_of(struct rio_link, link, link);
    int sleep_ms_rem = timeout_ms;
    printf("%s: poll: waiting for reply (timeout %u ms)...\r\n",
           link->name, timeout_ms);
    while (!rlink->cmd_ctx.reply_received && sleep_ms_rem) {
        msleep_and_dec(&sleep_ms_rem);
    }
    if (!rlink->cmd_ctx.reply_received) {
        return 1; /* timed out */
    }
    printf("%s: poll: reply received\r\n", link->name);
    return 0;
}

static int rio_link_send(struct link *link, int timeout_ms, void *buf,
                         size_t sz)
{
    struct rio_link *rlink = container_of(struct rio_link, link, link);
    int rc;
    ASSERT(link);
    DPRINTF("RIO LINK %s: send sz %u\r\n", link->name, sz);
    rc = rio_ep_msg_send(rlink->ep, rlink->dest,
                         rlink->mbox_remote, rlink->letter_remote,
                         rlink->seg_size, (uint8_t *)buf, sz);
    return rc ? 0 : sz; /* return code convention differs */
}

static int rio_link_request(struct link *link,
                             int wtimeout_ms, void *wbuf, size_t wsz,
                             int rtimeout_ms, void *rbuf, size_t rsz)
{
    struct rio_link *rlink = container_of(struct rio_link, link, link);
    int rc;

    DPRINTF("%s: request\r\n", link->name);

    rlink->cmd_ctx.reply = rbuf;
    rlink->cmd_ctx.reply_sz = rsz;
    rlink->cmd_ctx.reply_received = false;

    rc = rio_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printf("%s: request: send failed: rc %d\r\n", link->name, rc);
        return -1;
    }
    rc = rio_link_poll(link, rtimeout_ms);
    if (rc)
        printf("%s: request: wait failed: %d\r\n", link->name, rc);
    return rc;
}

int rio_link_disconnect(struct link *link)
{
    struct rio_link *rlink = container_of(struct rio_link, link, link);
    ASSERT(link);
    DPRINTF("RIO LINK %s: disconnect\r\n", link->name);
    rio_ep_unsub_msg_rx(rlink->ep);
    OBJECT_FREE(rlink);
    return 0;
}

struct link *rio_link_connect(const char *name, bool is_server,
                              struct rio_ep *ep,
                              unsigned mbox_local, unsigned letter_local,
                              rio_devid_t dest,
                              unsigned mbox_remote, unsigned letter_remote,
                              unsigned seg_size)
{
    DPRINTF("RIO LINK %s: connect\r\n", name);

    struct rio_link *rlink = OBJECT_ALLOC(links);
    if (!rlink)
        return NULL;
    rlink->ep = ep;
    rlink->mbox_local = mbox_local;
    rlink->letter_local = letter_local;
    rlink->dest = dest;
    rlink->mbox_remote = mbox_remote;
    rlink->letter_remote = letter_remote;
    rlink->seg_size = seg_size;

    struct link *link = &rlink->link;
    link->name = name;
    link->disconnect = rio_link_disconnect;
    link->send = rio_link_send;
    link->request = rio_link_request;
    link->recv = NULL;

    if (is_server) {
        rio_ep_sub_msg_rx(ep, handle_request, rlink);
    } else { /* client */
        rio_ep_sub_msg_rx(ep, handle_reply, rlink);
    }
    return link;
}
