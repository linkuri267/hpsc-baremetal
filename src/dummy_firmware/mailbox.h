#ifndef MAILBOX_H
#define MAILBOX_H

/* External IRQ numbering (i.e. vector #16 has index 0). */
#define MBOX_HAVE_DATA_IRQ 162

#define MBOX_BASE 0xF9240000

#define MBOX_REG_MAIL0 0x80
#define MBOX_REG_MAIL1 0xA0

void mbox_send(uint8_t msg);
uint8_t mbox_receive();

#endif // MAILBOX_H
