#include <stdint.h>

#include <libmspprintf/printf.h>

#include "mailbox.h"

void mbox_have_data_isr()
{
    volatile uint32_t *read_addr = (volatile uint32_t *)(MBOX_BASE + MBOX_REG_MAIL1_READ);
    uint32_t read_val = *read_addr;
    printf("%p -> %08lx\n", read_addr, read_val);
}
