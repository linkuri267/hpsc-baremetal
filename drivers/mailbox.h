#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>
#include <stdlib.h>

#include "intc.h"

#define HPSC_MBOX_AS_SIZE 0x10000 // address allocation in mem map/Qemu DT/Qemu model

#define HPSC_MBOX_DATA_REGS 16
#define HPSC_MBOX_DATA_SIZE (HPSC_MBOX_DATA_REGS * 4)

typedef void (*rcv_cb_t)(void *arg);
typedef void (*ack_cb_t)(void *arg);

enum mbox_dir {
        MBOX_OUTGOING = 0,
        MBOX_INCOMING = 1,
};

union mbox_cb {
    rcv_cb_t rcv_cb;
    ack_cb_t ack_cb;
};

struct mbox;

struct mbox *mbox_claim(volatile uint32_t * ip_base, unsigned instance,
                        struct irq *irq, unsigned int_idx,
                        uint32_t owner, uint32_t src, uint32_t dest,
                        enum mbox_dir dir, union mbox_cb cb, void *cb_arg);
int mbox_release(struct mbox *m);
size_t mbox_send(struct mbox *m, void *buf, size_t sz);
size_t mbox_read(struct mbox *m, void *buf, size_t sz);

void mbox_rcv_isr(unsigned int_idx);
void mbox_ack_isr(unsigned int_idx);

#endif // MAILBOX_H
