#include <stdint.h>

#include "console.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "hwinfo.h"
#include "gic.h"
#include "command.h"
#include "panic.h"
#include "arm.h"
#include "test.h"

int test_rtps_trch_mailbox()
{
#define LSIO_RCV_IRQ_IDX  MBOX_LSIO__RTPS_RCV_INT
#define LSIO_ACK_IRQ_IDX  MBOX_LSIO__RTPS_ACK_INT
    struct mbox_link_dev mdev;
    mdev.base = MBOX_LSIO__BASE;
    mdev.rcv_irq = gic_request(RTPS_IRQ__TR_MBOX_0 + LSIO_RCV_IRQ_IDX,
                               GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);
    mdev.rcv_int_idx = LSIO_RCV_IRQ_IDX;
    mdev.ack_irq = gic_request(RTPS_IRQ__TR_MBOX_0 + LSIO_ACK_IRQ_IDX,
                               GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);
    mdev.ack_int_idx = LSIO_ACK_IRQ_IDX;

    struct link *trch_link;
    switch (self_core_id()) {
        case 0:
            trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK",
                &mdev, MBOX_LSIO__TRCH_RTPS_R52_0, MBOX_LSIO__RTPS_R52_0_TRCH,
                /* server */ 0, /* client */ MASTER_ID_RTPS_CPU0);
            break;
        case 1:
            trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK",
                &mdev, MBOX_LSIO__TRCH_RTPS_R52_1, MBOX_LSIO__RTPS_R52_1_TRCH,
                /* server */ 0, /* client */ MASTER_ID_RTPS_CPU1);
            break;
        default:
            panic("invalid RTPS R52 core ID");
    }
    if (!trch_link)
        return 1;

    uint32_t arg[] = { CMD_PING, 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    printf("arg len: %u\r\n", sizeof(arg) / sizeof(arg[0]));
    int rc = trch_link->request(trch_link,
                                CMD_TIMEOUT_MS_SEND, arg, sizeof(arg),
                                CMD_TIMEOUT_MS_RECV, reply, sizeof(reply));
    if (rc <= 0)
        return rc;

    rc = trch_link->disconnect(trch_link);
    if (rc)
        return 1;

    return 0;
}
