#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "command.h"

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_size)
{
    size_t i;
    switch (cmd->msg[0]) {
        case CMD_NOP:
            // do nothing and reply nothing command
            return 0;
        case CMD_PING:
            printf("PING ...\r\n");
            reply[0] = CMD_PONG;
            for (i = 1; i < CMD_MSG_LEN && i < reply_size; ++i)
                reply[i] = cmd->msg[i];
            return i;
        case CMD_PONG:
            printf("PONG ...\r\n");
            return 0;
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->msg[0]);
            return -1;
    }
}
