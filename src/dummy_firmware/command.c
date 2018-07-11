#include <stdint.h>

#include <libmspprintf/printf.h>

#include "mailbox.h"
#include "reset.h"

#include "command.h"


void cmd_handle(unsigned cmd)
{
    printf("CMD %x\n", cmd);
    switch (cmd) {
        case CMD_RESET_HPPS:
            reset_hpps(/* first boot */ false);
            break;
        default:
            printf("ERROR: unknown cmd: %x\n", cmd);
    }
}
