#include <stdint.h>

#include "link.h"
#include "object.h"
#include "printf.h"
#include "shmem.h"
#include "sleep.h"

struct shmem_link {
    struct object obj;
    struct shmem *shmem_out;
    struct shmem *shmem_in;
};

#define MAX_LINKS 8
// pretty coarse, limited by systick
#define MIN_SLEEP_MS 500

static struct link links[MAX_LINKS] = {0};
static struct shmem_link slinks[MAX_LINKS] = {0};

static int shmem_link_disconnect(struct link *link)
{
    struct shmem_link *slink = link->priv;
    printf("%s: disconnect\r\n", link->name);
    shmem_close(slink->shmem_out);
    shmem_close(slink->shmem_in);
    OBJECT_FREE(slink);
    OBJECT_FREE(link);
    return 0;
}

static void msleep_and_dec(int *ms_rem)
{
    msleep(MIN_SLEEP_MS);
    // if < 0, timeout is infinite
    if (*ms_rem > 0)
        *ms_rem -= *ms_rem >= MIN_SLEEP_MS ? MIN_SLEEP_MS : *ms_rem;
}

static int shmem_link_send(struct link *link, int timeout_ms, void *buf,
                           size_t sz)
{
    struct shmem_link *slink = link->priv;
    int sleep_ms_rem = timeout_ms;
    do {
        if (!shmem_get_status(slink->shmem_out)) {
            return shmem_send(slink->shmem_out, buf, sz);
        }
        if (!sleep_ms_rem)
            break; // timeout
        msleep_and_dec(&sleep_ms_rem);
    } while (1);
    return 0;
}

static bool shmem_link_is_send_acked(struct link *link)
{
    struct shmem_link *slink = link->priv;
    // status is currently the "is_new" field, so not ACK'd until cleared
    return shmem_get_status(slink->shmem_out) ? false : true;
}

static int shmem_link_recv(struct link *link, void *buf, size_t sz)
{
    struct shmem_link *slink = link->priv;
    if (shmem_get_status(slink->shmem_in))
        return shmem_recv(slink->shmem_in, buf, sz);
    return 0;
}

static int shmem_link_poll(struct link *link, int timeout_ms, void *buf,
                           size_t sz)
{
    int sleep_ms_rem = timeout_ms;
    int rc;
    do {
        rc = shmem_link_recv(link, buf, sz);
        if (rc > 0)
            break; // got data
        if (!sleep_ms_rem)
            break; // timeout
        msleep_and_dec(&sleep_ms_rem);
    } while (1);
    return rc;
}

static int shmem_link_request(struct link *link,
                              int wtimeout_ms, void *wbuf, size_t wsz,
                              int rtimeout_ms, void *rbuf, size_t rsz)
{
    int rc;
    printf("%s: request\r\n", link->name);
    rc = shmem_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printf("shmem_link_request: send timed out\r\n");
        return -1;
    }
    rc = shmem_link_poll(link, rtimeout_ms, rbuf, rsz);
    if (!rc)
        printf("shmem_link_request: recv timed out\r\n");
    return rc;
}

struct link *shmem_link_connect(const char* name, void *addr_out, void *addr_in)
{
    struct shmem_link *slink;
    struct link *link;
    printf("%s: connect\r\n", name);
    printf("\taddr_out = 0x%x\r\n", (unsigned long) addr_out);
    printf("\taddr_in  = 0x%x\r\n", (unsigned long) addr_in);
    link = OBJECT_ALLOC(links);
    if (!link)
        return NULL;

    slink = OBJECT_ALLOC(slinks);
    if (!slink) {
        goto free_link;
    }
    slink->shmem_out = shmem_open(addr_out);
    if (!slink->shmem_out)
        goto free_links;
    slink->shmem_in = shmem_open(addr_in);
    if (!slink->shmem_in)
        goto free_out;

    link->priv = slink;
    link->name = name;
    link->disconnect = shmem_link_disconnect;
    link->send = shmem_link_send;
    link->is_send_acked = shmem_link_is_send_acked;
    link->request = shmem_link_request;
    link->recv = shmem_link_recv;
    return link;

free_out:
    shmem_close(slink->shmem_out);
free_links:
    OBJECT_FREE(slink);
free_link:
    OBJECT_FREE(link);
    return NULL;
}
