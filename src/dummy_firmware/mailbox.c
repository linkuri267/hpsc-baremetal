#include <stdint.h>

#include <libmspprintf/printf.h>

#include "mailbox.h"
#include "command.h"

// Mailbox has two slots:
//   Slot 0: outgoing direction (send writes to #0)
//   Slot 1: incoming direction (receive reads from #1)
// So neither endpoint ever reads and writes to the same slot.

// Layout of message inside a slot:
//   bits 4-8 bits are the payload
//   Bits 0-4 designate channel index (leftover from BCM2835 mbox)

#define OFFSET_PAYLOAD 4

void mbox_send(uint8_t msg)
{
    volatile uint32_t *slot = (volatile uint32_t *)(MBOX_BASE + MBOX_REG_MAIL0);
    uint32_t val = msg << OFFSET_PAYLOAD; // see layout above
    printf("%p <- %08lx\n", slot, val);
    *slot = val;
}

uint8_t mbox_receive()
{
    volatile uint32_t *slot = (volatile uint32_t *)(MBOX_BASE + MBOX_REG_MAIL1);
    uint32_t val = *slot;
    printf("%p -> %08lx\n", slot, val);
    uint8_t msg = val >> OFFSET_PAYLOAD; // see layout above
    return msg;
}

void mbox_have_data_isr()
{
    uint8_t msg = mbox_receive();
    printf("MBOX ISR: rcved msg %x\n", msg);
    cmd_handle(msg);
}
