#include <stdint.h>

#include "printf.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "hwinfo.h"
#include "command.h"
#include "test.h"

int test_rtps_trch_mailbox()
{
    struct mbox_link *rtps_link = mbox_link_connect(
                    MBOX_LSIO__BASE, MBOX_LSIO__IRQ_START,
                    MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                    MBOX_LSIO__RTPS_RCV_INT, MBOX_LSIO__RTPS_ACK_INT,
                    /* server */ 0,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        return 1;

    unsigned cmd = CMD_ECHO;
    uint32_t arg[] = { 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    printf("arg len: %u\r\n", sizeof(arg) / sizeof(arg[0]));
    int rc = mbox_link_request(rtps_link, cmd, arg, sizeof(arg) / sizeof(arg[0]),
                               reply, sizeof(reply) / sizeof(reply[0]));
    if (rc)
        return rc;

    rc = mbox_link_disconnect(rtps_link);
    if (rc)
        return 1;

    return 0;
}
