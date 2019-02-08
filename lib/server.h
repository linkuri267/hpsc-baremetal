#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdlib.h>

#include "command.h"

struct mbox_link_dev {
    volatile uint32_t *base;
    struct irq *rcv_irq;
    unsigned rcv_int_idx;
    struct irq *ack_irq;
    unsigned ack_int_idx;
};

int server_init(struct mbox_link_dev *devs, size_t ndevs);

// Compatible with cmd_handler_t function
int server_process(struct cmd *cmd, void *reply, size_t reply_sz);

#endif // SERVER_H
