#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <unistd.h>
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

typedef int (cmd_handler_t)(struct cmd *cmd, uint32_t *reply, size_t reply_size);

void cmd_handler_register(cmd_handler_t *cb);
void cmd_handler_unregister();

void cmd_handle(struct cmd *cmd);

int cmd_enqueue(struct cmd *cmd);
int cmd_dequeue(struct cmd *cmd);
bool cmd_pending();

#endif // COMMAND_H
