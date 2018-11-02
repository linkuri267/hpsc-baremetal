#ifndef SERVER_H
#define SERVER_H

#include "command.h"

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_len);

#endif // SERVER_H
