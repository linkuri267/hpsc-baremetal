#include <stdint.h>

#include "printf.h"
#include "mailbox-link.h"
#include "mailbox-map.h"
#include "hwinfo.h"
#include "gic.h"
#include "command.h"
#include "test.h"

int test_rtps_trch_mailbox()
{
#define LSIO_RCV_IRQ_IDX  MBOX_LSIO__RTPS_RCV_INT
#define LSIO_ACK_IRQ_IDX  MBOX_LSIO__RTPS_ACK_INT
#define TIMEOUT_MS_SEND 1000
#define TIMEOUT_MS_RECV 1000
    struct irq *lsio_rcv_irq =
        gic_request(RTPS_IRQ__TR_MBOX_0 + LSIO_RCV_IRQ_IDX,
                    GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);
    struct irq *lsio_ack_irq =
        gic_request(RTPS_IRQ__TR_MBOX_0 + LSIO_ACK_IRQ_IDX,
                    GIC_IRQ_TYPE_SPI, GIC_IRQ_CFG_LEVEL);

    struct link *rtps_link = mbox_link_connect(MBOX_LSIO__BASE,
                    MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                    lsio_rcv_irq, LSIO_RCV_IRQ_IDX,
                    lsio_ack_irq, LSIO_ACK_IRQ_IDX,
                    /* server */ 0,
                    /* client */ MASTER_ID_RTPS_CPU0);
    if (!rtps_link)
        return 1;

    uint32_t arg[] = { CMD_PING, 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    printf("arg len: %u\r\n", sizeof(arg) / sizeof(arg[0]));
    int rc = rtps_link->request(rtps_link, TIMEOUT_MS_SEND, arg, sizeof(arg),
                                TIMEOUT_MS_RECV, reply, sizeof(reply));
    if (rc)
        return rc;

    rc = rtps_link->disconnect(rtps_link);
    if (rc)
        return 1;

    return 0;
}
