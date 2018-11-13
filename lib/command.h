#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include "mailbox.h"

#define CMD_MSG_LEN 16

#define CMD_NOP                         0
#define CMD_PING                        1
#define CMD_PONG                        2
#define CMD_WATCHDOG_TIMEOUT            11
#define CMD_LIFECYCLE                   13
#define CMD_RESET_HPPS                  100
#define CMD_MBOX_LINK_CONNECT           1000
#define CMD_MBOX_LINK_DISCONNECT        1001
#define CMD_MBOX_LINK_PING              1002

struct cmd {
    uint32_t msg[CMD_MSG_LEN];
    struct mbox *reply_mbox;
    volatile bool *reply_acked;
};

void cmd_handle(struct cmd *cmd);

int cmd_enqueue(struct cmd *cmd);
int cmd_dequeue(struct cmd *cmd);

#endif // COMMAND_H
