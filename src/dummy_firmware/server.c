#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "reset.h"
#include "command.h"
#include "mailbox-link.h"

#define CMD_ECHO                        0x1
#define CMD_NOP                         0x2
#define CMD_RESET_HPPS                  0x3
#define CMD_MBOX_LINK_CONNECT           0x4
#define CMD_MBOX_LINK_DISCONNECT        0x5
#define CMD_MBOX_LINK_ECHO              0x6

#define ENDPOINT_HPPS 0
#define ENDPOINT_RTPS 1

#define MAX_MBOX_LINKS          8

static struct mbox_link *links[MAX_MBOX_LINKS];

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

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_size)
{
    unsigned i;
    switch (cmd->cmd) {
        case CMD_ECHO:
            printf("ECHO %x...\r\n", cmd->arg[0]);
            for (i = 0; i < MAX_CMD_ARG_LEN && i < reply_size; ++i)
                reply[i] = cmd->arg[i];
            return i;
        case CMD_NOP:
            // do nothing and reply nothing command
            return 0;
        case CMD_RESET_HPPS:
            reset_component(COMPONENT_HPPS);
            reply[0] = 0;
            return 1;
        case CMD_MBOX_LINK_CONNECT: {
            int rc;
            const char *endpoint_name;
            volatile uint32_t *base;
            switch (cmd->arg[0]) {
                case ENDPOINT_RTPS:
                    base = LSIO_MBOX_BASE;
                    endpoint_name = "RTPS";
                    break;
                case ENDPOINT_HPPS:
                    base = HPPS_MBOX_BASE;
                    endpoint_name = "HPPS";
                    break;
                default:
                    reply[0] = -1;
                    return 1;
            }

            struct mbox_link *link = mbox_link_connect(base,
                            /* from mbox */ cmd->arg[1], /* to mbox */ cmd->arg[2],
                            /* owner */ 0, /* dest  */ cmd->arg[3],
                            /* endpoint */ endpoint_name);
            if (!link) {
                rc = -2;
            } else {
                rc = linkp_alloc(link);
            }
            reply[0] = rc;
            return 1;
        }
        case CMD_MBOX_LINK_DISCONNECT: {
            int rc;
            unsigned index = cmd->arg[0];
            if (index >= MAX_MBOX_LINKS) {
                rc = -1;
            } else {
                linkp_free(index);
                rc = 0;
            }
            reply[0] = rc;
            return 1;
        }
        case CMD_MBOX_LINK_ECHO: {
            unsigned index = cmd->arg[0];
            if (index >= MAX_MBOX_LINKS) {
                reply[0] = -1;
                return 1;
            }

            struct mbox_link *link = links[index];
            uint32_t arg[] = { 43 };
            uint32_t reply[1];
            int rc = mbox_link_request(link, CMD_ECHO, arg, 1, reply, 1 + 1 /* cmd + arg */);
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
