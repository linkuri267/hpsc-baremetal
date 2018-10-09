#include <stdint.h>

#include "printf.h"
#include "mailbox.h"
#include "reset.h"

#include "command.h"

void cmd_handle(void *cbarg, volatile uint32_t *mbox_base, uint32_t *msg)
{
    unsigned cmd = msg[0];
    unsigned arg = msg[1];

    uint32_t reply[1];

    printf("CMD handle cmd %x arg %x\r\n", cmd, arg);

    switch (cmd) {
        case CMD_ECHO:
            printf("ECHO %x\r\n", arg);
            reply[0] = arg;
            mbox_reply(mbox_base, reply, 1);
            break;
        case CMD_RESET_HPPS:
            reset_component(COMPONENT_HPPS);
            reply[0] = 0;
            mbox_reply(mbox_base, reply, 1);
            break;
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd);
    }
}
