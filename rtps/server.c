#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "command.h"

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_size)
{
    unsigned i;
    switch (cmd->cmd) {
        case CMD_NOP:
            // do nothing and reply nothing command
            return 0;
        case CMD_PING:
            printf("PING %x...\r\n", cmd->arg[0]);
            reply[0] = CMD_PONG;
            for (i = 1; i < MAX_CMD_ARG_LEN && i < reply_size; ++i)
                reply[i] = cmd->arg[i];
            return i;
        case CMD_PONG:
            printf("PONG %x...\r\n", cmd->arg[0]);
            return 0;
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->cmd);
            return -1;
    }
}
