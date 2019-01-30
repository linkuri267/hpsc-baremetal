#include <stdint.h>
#include <unistd.h>

#include "printf.h"
#include "command.h"
#include "link.h"
#include "mailbox-link.h"
#include "hwinfo.h"
#include "boot.h"
#include "server.h"

#define ENDPOINT_HPPS 0
#define ENDPOINT_RTPS 1

#define MAX_MBOX_LINKS          8

static struct link *links[MAX_MBOX_LINKS] = {0};
static struct endpoint *endpoints = NULL;
static size_t num_endpoints = 0;

static int linkp_alloc(struct link *link)
{
    size_t i = 0;
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

int server_init(struct endpoint *endpts, size_t num_endpts)
{
    endpoints = endpts;
    num_endpoints = num_endpts;
    return 0;
}

int server_process(struct cmd *cmd, uint32_t *reply, size_t reply_size)
{
    size_t i;
    int rc;
    switch (cmd->msg[0]) {
        case CMD_NOP:
            printf("NOP ...\r\n");
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
        case CMD_WATCHDOG_TIMEOUT:
            printf("WATCHDOG_TIMEOUT ...\r\n");
            printf("\tCPU = %u\r\n", (unsigned int) cmd->msg[1]);
            return 0;
        case CMD_LIFECYCLE:
            printf("LIFECYCLE ...\r\n");
            printf("\tstatus = %s\r\n", cmd->msg[1] ? "DOWN" : "UP");
            printf("\tinfo = '%s'\r\n", (char*) &cmd->msg[2]);
            return 0;
        case CMD_RESET_HPPS:
            printf("RESET_HPPS ...\r\n");
            boot_request(SUBSYS_HPPS);
            reply[0] = 0;
            return 1;
        case CMD_MBOX_LINK_CONNECT: {
            unsigned endpoint_idx = cmd->msg[1];
            unsigned idx_from = cmd->msg[2];
            unsigned idx_to = cmd->msg[3];
            printf("MBOX_LINK_CONNECT ...\r\n");
            printf("\tendpoint_idx = %u\r\n", endpoint_idx);
            printf("\tindex_from = %u\r\n", idx_from);
            printf("\tindex_to = %u\r\n", idx_to);

            if (endpoint_idx >= num_endpoints) {
                reply[0] = -1;
                return 1;
            }
            struct endpoint *endpt = &endpoints[endpoint_idx];

            struct link *link = mbox_link_connect(endpt->base,
                            idx_from, idx_to,
                            endpt->rcv_irq, endpt->rcv_int_idx,
                            endpt->ack_irq, endpt->ack_int_idx,
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
            printf("MBOX_LINK_DISCONNECT ...\r\n");
            int rc;
            unsigned index = cmd->msg[1];
            if (index >= MAX_MBOX_LINKS) {
                rc = -1;
            } else {
                printf("link disconnect index: %u\r\n", index);
                rc = links[index]->disconnect(links[index]);
                linkp_free(index);
            }
            printf("link disconnect rc: %u\r\n", rc);
            reply[0] = rc;
            return 1;
        }
        case CMD_MBOX_LINK_PING: {
            unsigned index = cmd->msg[1];
            printf("MBOX_LINK_PING ...\r\n");
            printf("\tindex = %u\r\n", index);
            if (index >= MAX_MBOX_LINKS) {
                reply[0] = -1;
                return 1;
            }

            struct link *link = links[index];
            uint32_t msg[] = { CMD_PING, 43 };
            uint32_t reply[2];
            int rc = link->request(link,
                                   CMD_TIMEOUT_MS_SEND, msg, sizeof(msg),
                                   CMD_TIMEOUT_MS_RECV, reply, sizeof(reply));
            if (rc) {
                reply[0] = -2;
                return 1;
            }

            reply[0] = 0;
            return 1;
        }
        default:
            printf("ERROR: unknown cmd: %x\r\n", cmd->msg[0]);
            return -1;
    }
}
