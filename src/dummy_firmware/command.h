#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include "mailbox.h"

#define CMD_ECHO       0x1
#define CMD_RESET_HPPS 0x3

struct cmd {
    uint32_t cmd;
    uint32_t arg;
    struct mbox *reply_mbox;
    volatile bool *reply_acked;
};

void cmd_handle(struct cmd *cmd);

int cmd_enqueue(struct cmd *cmd);
int cmd_dequeue(struct cmd *cmd);

#endif // COMMAND_H
