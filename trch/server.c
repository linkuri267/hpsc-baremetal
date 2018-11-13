#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "reset.h"
#include "command.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "hwinfo.h"

#define ENDPOINT_HPPS 0
#define ENDPOINT_RTPS 1

#define MAX_MBOX_LINKS          8

static struct mbox_link *links[MAX_MBOX_LINKS] = {0};

static int linkp_alloc(struct mbox_link *link)
{
    unsigned i = 0;
    while (links[i] && i < MAX_MBOX_LINKS)
        ++i;
    if (i == MAX_MBOX_LINKS)
        return -1;
    links[i] = link;
    return i;
}

static void linkp_free(int index)
{
     links[index] = NULL;
}

static const char *cmd_to_str(unsigned cmd)
{
    switch (cmd) {
        case CMD_NOP:                   return "NOP";
        case CMD_PING:                  return "PING";
        case CMD_PONG:                  return "PONG";
        case CMD_WATCHDOG_TIMEOUT:      return "WATCHDOG_TIMEOUT";
        case CMD_LIFECYCLE:             return "LIFECYCLE";
        case CMD_RESET_HPPS:            return "RESET_HPPS";
        case CMD_MBOX_LINK_CONNECT:     return "MBOX_LINK_CONNECT";
        case CMD_MBOX_LINK_DISCONNECT:  return "MBOX_LINK_DISCONNECT";
        case CMD_MBOX_LINK_PING:        return "MBOX_LINK_PING";
        default:                        return "?";
    }
}

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_size)
{
    unsigned i;
    printf("Processing CMD: %s\r\n", cmd_to_str(cmd->cmd));
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
        case CMD_WATCHDOG_TIMEOUT:
            printf("WATCHDOG_TIMEOUT %x...\r\n", cmd->arg[0]);
            printf("\tCPU = %u\r\n", (unsigned int) cmd->arg[0]);
            return 0;
        case CMD_LIFECYCLE:
            printf("LIFECYCLE %x...\r\n", cmd->arg[0]);
            printf("\tstatus = %s\r\n", cmd->arg[0] ? "DOWN" : "UP");
            printf("\tinfo = '%s'\r\n", (char*) &cmd->arg[1]);
            return 0;
        case CMD_RESET_HPPS:
            reset_component(COMPONENT_HPPS);
            reply[0] = 0;
            return 1;
        case CMD_MBOX_LINK_CONNECT: {
            int rc;
            volatile uint32_t *base;
            unsigned irq_base;
            unsigned rcv_int_idx, ack_int_idx;
            switch (cmd->arg[0]) {
                case ENDPOINT_RTPS:
                    base = MBOX_LSIO__BASE;
                    irq_base = MBOX_LSIO__IRQ_START;
                    rcv_int_idx = MBOX_LSIO__TRCH_RCV_INT;
                    ack_int_idx = MBOX_LSIO__TRCH_ACK_INT;
                    break;
                case ENDPOINT_HPPS:
                    base = MBOX_HPPS_TRCH__BASE;
                    irq_base = MBOX_HPPS_TRCH__IRQ_START;
                    rcv_int_idx = MBOX_HPPS_TRCH__TRCH_RCV_INT;
                    ack_int_idx = MBOX_HPPS_TRCH__TRCH_ACK_INT;
                    break;
                default:
                    reply[0] = -1;
                    return 1;
            }

            struct mbox_link *link = mbox_link_connect(base, irq_base,
                            /* from mbox */ cmd->arg[1], /* to mbox */ cmd->arg[2],
                            rcv_int_idx, ack_int_idx,
                            /* server */ 0, /* client */ MASTER_ID_TRCH_CPU);
            if (!link) {
                rc = -2;
            } else {
                rc = linkp_alloc(link);
            }
            printf("link connect rc: %u\r\n", rc);
            reply[0] = rc;
            return 1;
        }
        case CMD_MBOX_LINK_DISCONNECT: {
            int rc;
            unsigned index = cmd->arg[0];
            if (index >= MAX_MBOX_LINKS) {
                rc = -1;
            } else {
                rc = mbox_link_disconnect(links[index]);
                linkp_free(index);
            }
            printf("link disconnect rc: %u\r\n", rc);
            reply[0] = rc;
            return 1;
        }
        case CMD_MBOX_LINK_PING: {
            unsigned index = cmd->arg[0];
            if (index >= MAX_MBOX_LINKS) {
                reply[0] = -1;
                return 1;
            }

            struct mbox_link *link = links[index];
            uint32_t arg[] = { 43 };
            uint32_t reply[1];
            int rc = mbox_link_request(link, CMD_PING, arg, 1, reply, 1 + 1 /* cmd + arg */);
            if (rc) {
                reply[0] = -2;
                return 1;
            }

            reply[0] = 0;
            return 1;
        }
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->cmd);
            return -1;
    }
}
