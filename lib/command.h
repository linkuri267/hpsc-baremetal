#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <unistd.h>
#include "link.h"

#define CMD_MSG_SZ 64
#define CMD_MSG_PAYLOAD_OFFSET 4

#define CMD_NOP                         0
#define CMD_PING                        1
#define CMD_PONG                        2
#define CMD_WATCHDOG_TIMEOUT            11
#define CMD_LIFECYCLE                   13
#define CMD_ACTION                      14
#define CMD_MBOX_LINK_CONNECT           200
#define CMD_MBOX_LINK_DISCONNECT        201
#define CMD_MBOX_LINK_PING              202

#define CMD_ACTION_RESET_HPPS           1

#define CMD_TIMEOUT_MS_SEND 1000
#define CMD_TIMEOUT_MS_RECV 1000
// wait up to 30 seconds for replies - a timeout prevent hangs when remotes fail
#define CMD_TIMEOUT_MS_REPLY 30000

struct cmd {
    // the first byte of the message is the type, the next 3 bytes are reserved
    // the remainder of the msg is avaialble for the payload
    uint8_t msg[CMD_MSG_SZ];
    struct link *link;
};

typedef int (cmd_handler_t)(struct cmd *cmd, void *reply, size_t reply_sz);

void cmd_handler_register(cmd_handler_t *cb);
void cmd_handler_unregister();

void cmd_handle(struct cmd *cmd);

int cmd_enqueue(struct cmd *cmd);
int cmd_dequeue(struct cmd *cmd);
bool cmd_pending();

#endif // COMMAND_H
