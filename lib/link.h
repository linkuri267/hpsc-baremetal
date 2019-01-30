#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/**
 * The link struct is effectively an API, which can be populated by other
 * link-like interfaces.
 */
struct link {
    void *priv;
    int (*disconnect)(struct link *link);
    int (*send)(struct link *link, int timeout_ms, void *buf, size_t sz);
    bool (*is_send_acked)(struct link *link);
    // int (*recv)(struct link *link, int timeout_ms, void *buf, size_t sz);
    int (*request)(struct link *link,
                   int wtimeout_ms, void *wbuf, size_t wsz,
                   int rtimeout_ms, void *rbuf, size_t rsz);
};

#endif // LINK_H
