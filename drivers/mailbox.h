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

struct mbox *mbox_claim(uintptr_t ip_base, unsigned instance,
                        struct irq *irq, unsigned int_idx,
                        uint32_t owner, uint32_t src, uint32_t dest,
                        enum mbox_dir dir, union mbox_cb cb, void *cb_arg);
int mbox_release(struct mbox *m);

/* 
 * Data read/write is kept separate from event set/clear b/c correct event
 * management ordering cannot be guaranteed by mbox_send/mbox_read and the ISRs
 * in the driver, without a lot of assumptions about callback routine behavior
 * and how mailboxes are used in general---and it's not pretty.
 * Higher-level abstractions can make event handling opaque, if desired.
 *
 * In the case of polled operation (not currently supported), event handling can
 * be performed at any time.
 * In the case where events drive IRQs (this design), events should be cleared
 * before ISRs complete, o/w the IRQ remains active and the ISR runs again.
 *
 * The driver _could_ enforce correct event handling by performing the read in
 * the driver's RCV ISR and passing the data pointer to the `rcv_cb` routine.
 * This requires either that `rcv_cb` handles the message before returning, or
 * that it copies the message somewhere more persistent, in which case there is
 * a wasteful data copy. Instead, we allow the next level up to read directly
 * into whatever buffer is desired, but require that it manage the events.
 */

/*
 * A note on correct usage:
 * The RCV event _must_ be cleared _before_ ACK is set, otherwise the sender may
 * receive the ACK and send a new message before the RCV event is actually
 * cleared, in which case the second message is missed.
 */

size_t mbox_send(struct mbox *m, void *buf, size_t sz);
size_t mbox_read(struct mbox *m, void *buf, size_t sz);

void mbox_event_set_rcv(struct mbox *m);
void mbox_event_set_ack(struct mbox *m);
void mbox_event_clear_rcv(struct mbox *m);
void mbox_event_clear_ack(struct mbox *m);

void mbox_rcv_isr(unsigned int_idx);
void mbox_ack_isr(unsigned int_idx);

#endif // MAILBOX_H
