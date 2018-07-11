#include <stdint.h>

#include <libmspprintf/printf.h>

#include "mailbox.h"
#include "command.h"

void mbox_have_data_isr()
{
    volatile uint32_t *read_addr = (volatile uint32_t *)(MBOX_BASE + MBOX_REG_MAIL1_READ);
    uint32_t read_val = *read_addr;
    printf("%p -> %08lx\n", read_addr, read_val);

    // lower 4 bits designate channel index (leftover from BCM2835 mbox)
    // upper 4 bits are the payload
    unsigned cmd = (read_val & 0xf0) >> 4;
    cmd_handle(cmd);
}
