#ifndef MAILBOX_LINK_H
#define MAILBOX_LINK_H

#include <stdint.h>

#include "intc.h"
#include "link.h"

struct mbox_link_dev {
    volatile uint32_t *base;
    struct irq *rcv_irq;
    unsigned rcv_int_idx; /* interrupt index within IP block */
    struct irq *ack_irq;
    unsigned ack_int_idx;
};

// We use 'owner' to indicate both the ID (arbitrary value) to which the
// mailbox should be claimed (i.e. OWNER HW register should be set) and whether
// the connection originator is a server or a client: owner!=0 ==> server;
// owner=0 ==> client. However, these concepts are orthogonal, so it would be
// easy to decouple them if desired by adding another arg to this function.
//
// To claim as server: set both server and client to non-zero ID
// To claim as client: set server to 0 and set client to non-zero ID
struct link *mbox_link_connect(const char *name, struct mbox_link_dev *ldev,
                               unsigned idx_from, unsigned idx_to,
                               unsigned server, unsigned client);

#endif // MAILBOX_LINK_H
