#include <stdint.h>

#include "mailbox-link.h"
#include "pwrmgr.h"
#include "subsys.h"
#include "panic.h"
#include "command.h"

static enum pm_node_id comp_to_node(comp_t comp)
{
    switch (comp) {
        case COMP_CPU_TRCH: panic("cannot do PSCI on TRCH"); return 0;
        case COMP_CPU_RTPS_R52_0: return NODE_RPU_0;
        case COMP_CPU_RTPS_R52_1: return NODE_RPU_1;
        case COMP_CPU_RTPS_A53_0: return NODE_RPU_A53;
        case COMP_CPU_HPPS_0: return NODE_APU_0;
        case COMP_CPU_HPPS_1: return NODE_APU_1;
        case COMP_CPU_HPPS_2: return NODE_APU_2;
        case COMP_CPU_HPPS_3: return NODE_APU_3;
        case COMP_CPU_HPPS_4: return NODE_APU_4;
        case COMP_CPU_HPPS_5: return NODE_APU_5;
        case COMP_CPU_HPPS_6: return NODE_APU_6;
        case COMP_CPU_HPPS_7: return NODE_APU_7;
        default: panic("invalid component id");
    }
}

static int psci_request(struct link *psci_link,
                        uint32_t *req, unsigned req_len,
                        uint32_t *reply, unsigned reply_size)
{
    int len;
    int rc;

    ASSERT(psci_link);
    len = psci_link->request(psci_link,
                             CMD_TIMEOUT_MS_SEND, req, req_len,
                             CMD_TIMEOUT_MS_RECV, reply, reply_size);
    if (len <= 0)
        return 1;

    ASSERT(len > 0);
    rc = reply[0];
    if (rc != PM_RET_SUCCESS) {
        printf("ERROR: PSCI request failed: rc %u\r\n", rc);
        return 1;
    }
    return 0;
}

int psci_release_reset(struct link *psci_link, comp_t requester, comp_t comp)
{
    int rc;
    uint32_t arg[] = { CMD_PSCI, comp_to_node(requester),
                       PM_REQ_WAKEUP, comp_to_node(comp),
                       0x0, 0x0, 0x0};
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    return psci_request(psci_link, arg, sizeof(arg), reply, sizeof(reply));
}
