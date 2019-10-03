#include <stdbool.h>
#include <stdint.h>

#include "intc.h"
#include "mailbox.h"
#include "object.h"
#include "panic.h"
#include "printf.h"
#include "regops.h"

#define REG_CONFIG              0x00
#define REG_EVENT_CAUSE         0x04
#define REG_EVENT_CLEAR         0x04
#define REG_EVENT_STATUS        0x08
#define REG_EVENT_SET           0x08
#define REG_INT_ENABLE          0x0C
#define REG_DATA                0x10

#define REG_CONFIG__UNSECURE      0x1
#define REG_CONFIG__OWNER__SHIFT  8
#define REG_CONFIG__OWNER__MASK   0x0000ff00
#define REG_CONFIG__SRC__SHIFT    16
#define REG_CONFIG__SRC__MASK     0x00ff0000
#define REG_CONFIG__DEST__SHIFT   24
#define REG_CONFIG__DEST__MASK    0xff000000

#define HPSC_MBOX_EVENT_A 0x1
#define HPSC_MBOX_EVENT_B 0x2
#define HPSC_MBOX_EVENT_C 0x4

#define HPSC_MBOX_INT_A(idx) (1 << (2 * (idx)))      // rcv (map event A to int 'idx')
#define HPSC_MBOX_INT_B(idx) (1 << (2 * (idx) + 1))  // ack (map event B to int 'idx')

#define HPSC_MBOX_EVENTS 2
#define HPSC_MBOX_INTS   16
#define HPSC_MBOX_INSTANCES 32
#define HPSC_MBOX_INSTANCE_REGION (REG_DATA + HPSC_MBOX_DATA_SIZE)

#define MAX_BLOCKS 2
#define MAX_MBOXES (MAX_BLOCKS * HPSC_MBOX_INSTANCES)

struct mbox_ip_block {
        struct object obj;
        uintptr_t base;
        unsigned refcnt;
        unsigned irq_refcnt[HPSC_MBOX_EVENTS];
};

struct mbox {
        struct object obj;
        struct mbox_ip_block *block;
        uintptr_t base;
        unsigned instance;
        int int_idx;
        struct irq *irq;
        bool owner; // whether this mailbox was claimed as owner
        union mbox_cb cb;
        void *cb_arg;
};

// The mboxes array is common across all mbox_ip_block's. We could let each
// block own its own mboxes array, and iterate over blocks in the ISR. Meh.
static struct mbox mboxes[MAX_MBOXES] = {0};
static struct mbox_ip_block blocks[MAX_BLOCKS] = {0};

static void mbox_irq_subscribe(struct mbox *mbox)
{
    if (mbox->block->irq_refcnt[mbox->int_idx]++ == 0)
        intc_int_enable(mbox->irq);
}
static void mbox_irq_unsubscribe(struct mbox *mbox)
{
    if (--mbox->block->irq_refcnt[mbox->int_idx] == 0)
        intc_int_disable(mbox->irq);
}

static struct mbox_ip_block *block_get(uintptr_t ip_base)
{
    struct mbox_ip_block *b;
    unsigned block = 0;
    while (block < MAX_BLOCKS &&
           (!blocks[block].obj.valid || blocks[block].base != ip_base))
        ++block;
    if (block == MAX_BLOCKS) { // no match
        b = OBJECT_ALLOC(blocks);
        if (!b)
            return NULL;
        b->base = ip_base;
    } else {
        b = &blocks[block];
    }
    ++b->refcnt;
    return b;
}
static void block_put(struct mbox_ip_block *b)
{
    ASSERT(b);
    ASSERT(b->refcnt);
    if (!--b->refcnt) {
        for (unsigned e = 0; e < HPSC_MBOX_EVENTS; ++e)
            ASSERT(!b->irq_refcnt[e]);
        OBJECT_FREE(b);
    }
}

struct mbox *mbox_claim(uintptr_t ip_base, unsigned instance,
                        struct irq *irq, unsigned int_idx,
                        uint32_t owner, uint32_t src, uint32_t dest,
                        enum mbox_dir dir, union mbox_cb cb, void *cb_arg)
{
    uint32_t cfg;
    uint32_t cfg_hw;
    uint32_t src_hw;
    uint32_t dest_hw;
    uint32_t ie;

    printf("mbox_claim: ip %x instance %u irq (type %u) %u int %u owner %x src %x dest %x dir %u\r\n",
           ip_base, instance, intc_int_type(irq), intc_int_num(irq),
           int_idx, owner, src, dest, dir);

    struct mbox *m = OBJECT_ALLOC(mboxes);
    if (!m)
        return NULL;

    m->block = block_get(ip_base);
    if (!m->block)
        goto cleanup;

    m->instance = instance;
    m->base = ip_base + instance * HPSC_MBOX_INSTANCE_REGION;
    m->int_idx = int_idx;
    m->irq = irq;
    m->owner = (owner != 0);

    if (m->owner) {
        cfg = REG_CONFIG__UNSECURE |
            ((owner << REG_CONFIG__OWNER__SHIFT) & REG_CONFIG__OWNER__MASK) |
            ((src << REG_CONFIG__SRC__SHIFT)     & REG_CONFIG__SRC__MASK) |
            ((dest  << REG_CONFIG__DEST__SHIFT)  & REG_CONFIG__DEST__MASK);
        printf("mbox_claim: config <- %08lx\r\n", cfg);
        REGB_WRITE32(m->base, REG_CONFIG, cfg);
        cfg_hw = REGB_READ32(m->base, REG_CONFIG);
        printf("mbox_claim: config -> %08lx\r\n", cfg_hw);
        if (cfg_hw != cfg) {
            printf("mbox_claim: failed to claim mailbox %u for %lx: already owned by %lx\r\n",
                   instance, owner, (cfg_hw & REG_CONFIG__OWNER__MASK) >> REG_CONFIG__OWNER__SHIFT);
            goto cleanup;
        }
    } else { // not owner, just check the value in registers against the requested value
        cfg_hw = REGB_READ32(m->base, REG_CONFIG);
        printf("mbox_claim: config -> %08lx\r\n", cfg_hw);
        src_hw =  (cfg_hw & REG_CONFIG__SRC__MASK) >> REG_CONFIG__SRC__SHIFT;
        dest_hw = (cfg_hw & REG_CONFIG__DEST__MASK) >> REG_CONFIG__DEST__SHIFT;
        if ((dir == MBOX_OUTGOING && src  && src_hw != src) ||
            (dir == MBOX_INCOMING && dest && dest_hw != src)) {
            printf("mbox_claim: failed to claim (instance %u dir %u): "
                   "src/dest mismatch: %lx/%lx (expected %lx/%lx)\r\n",
                   instance, dir, src, dest, src_hw, dest_hw);
            goto cleanup;
        }
    }

    m->cb = cb;
    m->cb_arg = cb_arg;

    switch (dir) {
        case MBOX_INCOMING:
            ie = HPSC_MBOX_INT_A(m->int_idx);
            break;
        case MBOX_OUTGOING:
            ie = HPSC_MBOX_INT_B(m->int_idx);
            break;
        default:
            printf("mbox_claim: invalid direction: %u\r\n", dir);
            goto cleanup;
    }

    printf("mbox_claim: int en <- %08lx\r\n", ie);
    REGB_SET32(m->base, REG_INT_ENABLE, ie);
    mbox_irq_subscribe(m);

    return m;
cleanup:
    OBJECT_FREE(m);
    return NULL;
}

int mbox_release(struct mbox *m)
{
    // We are the OWNER, so we can release
    static const uint32_t cfg = 0;
    printf("mbox_release: base %p instance %u\r\n", m->base, m->instance);
    if (m->owner) {
        printf("mbox_release: config <- %08lx\r\n", cfg);
        REGB_WRITE32(m->base, REG_CONFIG, cfg);
        // clearing owner also clears destination (resets the instance)
    }
    mbox_irq_unsubscribe(m);
    block_put(m->block);
    OBJECT_FREE(m);
    return 0;
}

size_t mbox_send(struct mbox *m, void *buf, size_t sz)
{
    unsigned i;
    uint32_t *msg = buf;
    unsigned len = sz / sizeof(uint32_t);
    ASSERT(sz <= HPSC_MBOX_DATA_SIZE);
    if (sz % sizeof(uint32_t))
        len++;

    printf("mbox_send: base %p instance %u\r\n", m->base, m->instance);
    printf("mbox_send: msg: ");
    for (i = 0; i < len; ++i) {
        REGB_WRITE32(m->base, REG_DATA + (i * sizeof(uint32_t)), msg[i]);
        printf("%x ", msg[i]);
    }
    printf("\r\n");
    // zero out any remaining registers
    for (; i < HPSC_MBOX_DATA_REGS; i++)
        REGB_WRITE32(m->base, REG_DATA + (i * sizeof(uint32_t)), 0);

    uint32_t val = HPSC_MBOX_EVENT_A;
    printf("mbox_send: raise int A <- %08lx\r\n", val);
    REGB_WRITE32(m->base, REG_EVENT_SET, val);

    return sz;
}

size_t mbox_read(struct mbox *m, void *buf, size_t sz)
{
    size_t i;
    uint32_t *msg = buf;
    size_t len = sz / sizeof(uint32_t);
    if (sz % sizeof(uint32_t))
        len++;

    printf("mbox_read: base %p instance %u\r\n", m->base, m->instance);
    printf("mbox_read: msg: ");
    for (i = 0; i < len && i < HPSC_MBOX_DATA_REGS; i++) {
        msg[i] = REGB_READ32(m->base, REG_DATA + (i * sizeof(uint32_t)));
        printf("%x ", msg[i]);
    }
    printf("\r\n");

    // ACK
    uint32_t val = HPSC_MBOX_EVENT_B;
    printf("mbox_read: set int B <- %08lx\r\n", val);
    REGB_WRITE32(m->base, REG_EVENT_SET, val);

    return i * sizeof(uint32_t);
}

static void mbox_instance_rcv_isr(struct mbox *mbox)
{
    uint32_t val = HPSC_MBOX_EVENT_A;
    printf("mbox_instance_rcv_isr: base %p instance %u\r\n", mbox->base, mbox->instance);

    // Clear the event first
    printf("mbox_instance_rcv_isr: clear int A <- %08lx\r\n", val);
    REGB_WRITE32(mbox->base, REG_EVENT_CLEAR, val);

    if (mbox->cb.rcv_cb)
        mbox->cb.rcv_cb(mbox->cb_arg);
}

static void mbox_instance_ack_isr(struct mbox *mbox)
{
    uint32_t val = HPSC_MBOX_EVENT_B;
    printf("mbox_instance_ack_isr: base %p instance %u\r\n", mbox->base, mbox->instance);

    // Clear the event first    
    printf("mbox_instance_ack_isr: clear int B <- %08lx\r\n", val);
    REGB_WRITE32(mbox->base, REG_EVENT_CLEAR, val);

    if (mbox->cb.ack_cb)
        mbox->cb.ack_cb(mbox->cb_arg);
}

static void mbox_isr(unsigned event, unsigned interrupt)
{
    uint32_t val;
    struct mbox *mbox;
    unsigned i;
    bool handled = false;

    // Technically, could iterate only over one IP block if we care to split
    // the main mailbox array into multiple arrays, one per block.
    for (i = 0; i < MAX_MBOXES; ++i) {
        mbox = &mboxes[i];
        if (!mbox->obj.valid)
            continue;

        // Are we 'signed up' for this event (A) from this mailbox (i)?
        // Two criteria: (1) Cause is set, and (2) Mapped to our IRQ
        val = REGB_READ32(mbox->base, REG_EVENT_CAUSE);
        printf("mbox_isr: cause -> %08lx\r\n", val);
        if (!(val & event))
            continue; // this mailbox didn't raise the interrupt
        val = REGB_READ32(mbox->base, REG_INT_ENABLE);
        printf("mbox_isr: int enable -> %08lx\r\n", val);
        if (!(val & interrupt))
            continue; // this mailbox has an event but it's not ours

        handled = true;

        switch (event) {
            case HPSC_MBOX_EVENT_A:
                mbox_instance_rcv_isr(mbox);
                break;
            case HPSC_MBOX_EVENT_B:
                mbox_instance_ack_isr(mbox);
                break;
            default:
                printf("ERROR: mbox_isr: invalid event %u\r\n", event);
                ASSERT(false && "invalid event");
        }
   }
   ASSERT(handled); // otherwise, we're not correctly subscribed to interrupts
}

void mbox_rcv_isr(unsigned int_idx)
{
    mbox_isr(HPSC_MBOX_EVENT_A, HPSC_MBOX_INT_A(int_idx));
}
void mbox_ack_isr(unsigned int_idx)
{
    mbox_isr(HPSC_MBOX_EVENT_B, HPSC_MBOX_INT_B(int_idx));
}
