#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "reset.h"
#include "command.h"

#define CMD_ECHO       0x1
#define CMD_RESET_HPPS 0x3

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_len)
{
    switch (cmd->cmd) {
        case CMD_ECHO:
            printf("ECHO %x\r\n", cmd->arg);
            reply[0] = cmd->arg;
            break;
        case CMD_RESET_HPPS:
            reset_component(COMPONENT_HPPS);
            reply[0] = 0;
            break;
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->cmd);
            return 1;
    }

   return 0;
}
