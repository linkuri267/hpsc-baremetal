#ifndef MAILBOX_LINK_H
#define MAILBOX_LINK_H

#include <stdint.h>

#include "intc.h"
#include "link.h"

// An abstract device which describes the mailbox IP block properties and
// configured interrupts that cover both inbound and outbound mailbox channels.
// These properties are configured externally from the mailbox driver and
// mailbox-link library, hence the separate data structure.
struct mbox_link_dev {
    uintptr_t base;
    struct irq *rcv_irq;
    unsigned rcv_int_idx; /* interrupt index within IP block */
    struct irq *ack_irq;
    unsigned ack_int_idx;
};

// It would be nice to keep mailbox device tracking more dynamic, but without
// advanced data structures readily available, we'll settle for a static count
typedef enum {
    MBOX_DEV_HPPS = 0,
    MBOX_DEV_LSIO,
    MBOX_DEV_COUNT
} mbox_dev_id;

int mbox_link_dev_add(mbox_dev_id id, struct mbox_link_dev *dev);

void mbox_link_dev_remove(mbox_dev_id id);

struct mbox_link_dev *mbox_link_dev_get(mbox_dev_id id);

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
