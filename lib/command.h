#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <unistd.h>
#include "link.h"

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

#define CMD_TIMEOUT_MS_SEND 1000
#define CMD_TIMEOUT_MS_RECV 1000
// wait up to 30 seconds for replies - a timeout prevent hangs when remotes fail
#define CMD_TIMEOUT_MS_REPLY 30000

struct cmd {
    uint32_t msg[CMD_MSG_LEN];
    struct link *link;
};

typedef int (cmd_handler_t)(struct cmd *cmd, uint32_t *reply, size_t reply_size);

void cmd_handler_register(cmd_handler_t *cb);
void cmd_handler_unregister();

void cmd_handle(struct cmd *cmd);

int cmd_enqueue(struct cmd *cmd);
int cmd_dequeue(struct cmd *cmd);
bool cmd_pending();

#endif // COMMAND_H
