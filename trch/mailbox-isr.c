#include "mailbox.h"
#include "mailbox-map.h"
#include "printf.h"


void mbox_lsio_rcv_isr()
{
    mbox_rcv_isr(MBOX_LSIO_TRCH_RCV_INT);
}
void mbox_lsio_ack_isr()
{
    mbox_ack_isr(MBOX_LSIO_TRCH_ACK_INT);
}
void mbox_hpps_rcv_isr()
{
    mbox_rcv_isr(MBOX_HPPS_TRCH_RCV_INT);
}
void mbox_hpps_ack_isr()
{
    mbox_ack_isr(MBOX_HPPS_TRCH_ACK_INT);
}
