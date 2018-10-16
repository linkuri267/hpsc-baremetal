#include <stdint.h>

#include "printf.h"
#include "mailbox.h"
#include "reset.h"

#include "command.h"

#define CMD_QUEUE_LEN 2

static unsigned cmdq_head = 0;
static unsigned cmdq_tail = 0;
static struct cmd cmdq[CMD_QUEUE_LEN];

int cmd_enqueue(struct cmd *cmd)
{
    if (cmdq_head + 1 % CMD_QUEUE_LEN == cmdq_tail) {
        printf("cannot enqueue command: queue full\r\n");
        return 1;
    }
    cmdq_head = (cmdq_head + 1) % CMD_QUEUE_LEN;
    cmdq[cmdq_head] = *cmd;
    printf("enqueue command (tail %u head %u): cmd %u arg %u\r\n",
           cmdq_tail, cmdq_head, cmdq[cmdq_head].cmd, cmdq[cmdq_head].arg);
    return 0;
}

int cmd_dequeue(struct cmd *cmd)
{
    if (cmdq_head == cmdq_tail)
        return 1;

    cmdq_tail = (cmdq_tail + 1) % CMD_QUEUE_LEN;
    *cmd = cmdq[cmdq_tail];
    printf("dequeue command (tail %u head %u): cmd %u arg %u\r\n",
           cmdq_tail, cmdq_head, cmdq[cmdq_tail].cmd, cmdq[cmdq_tail].arg);
    return 0;
}

void cmd_handle(struct cmd *cmd)
{
    uint32_t reply[1];

    printf("CMD handle cmd %x arg %x\r\n", cmd->cmd, cmd->arg);

    switch (cmd->cmd) {
        case CMD_ECHO:
            printf("ECHO %x\r\n", cmd->arg);
            reply[0] = cmd->arg;

            *cmd->reply_acked = false;
            if (mbox_send(cmd->reply_mbox, reply, 1)) {
                printf("failed to send reply\r\n");
            } else {
                printf("waiting for ACK for our reply\r\n");
                while(!*cmd->reply_acked); // block
                printf("ACK for our reply received\r\n");
            }
            break;
        case CMD_RESET_HPPS:
            reset_component(COMPONENT_HPPS);
            //reply[0] = 0;
            // mbox_reply(mbox_base, reply, 1); // TODO
            break;
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->cmd);
    }
}
