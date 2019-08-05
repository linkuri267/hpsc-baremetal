#ifndef RIO_H
#define RIO_H

#include <stdint.h>

#define RIO_MAX_PAYLOAD_SIZE 256 /* TODO: check */

struct rio_ep {
    const char *name;
    volatile uint32_t *base;
};

enum rio_transport_type {
    RIO_TRANSPORT_DEV8,
    RIO_TRANSPORT_DEV16,
    RIO_TRANSPORT_DEV32, /* not supported by Praesum BRC1 EP */
};

/* Talbe 4-1 in RIO Spec P1 */
enum rio_ftype {
    RIO_FTYPE_REQ           =  2,
    RIO_FTYPE_WRITE         =  5,
    RIO_FTYPE_STREAM_WRITE  =  6,
    RIO_FTYPE_MAINT         =  8,
    RIO_FTYPE_RESP          = 13,
};

enum rio_transaction {
    RIO_TRANS_MAINT_REQ_READ        = 0b0000,
    RIO_TRANS_MAINT_REQ_WRITE       = 0b0001,
    RIO_TRANS_MAINT_RESP_READ       = 0b0010,
    RIO_TRANS_MAINT_RESP_WRITE      = 0b0011,
    RIO_TRANS_MAINT_REQ_PORT_WRITE  = 0b0100,
};

/* Max supported by Praesum BRC1 EP are 16-bit IDs */
typedef uint16_t rio_devid_t;

/* Struct sufficiently general to hold a packet of any type */
/* TODO: consider splitting into physical, logical, and transport? */
struct rio_pkt {
    /* Physical Layer */
    uint8_t prio;

    /* Transport Layer */
    enum rio_transport_type ttype;
    rio_devid_t dest_id;
    rio_devid_t src_id;

    /* Logical Layer */
    enum rio_ftype ftype;
    uint8_t src_tid;
    uint8_t target_tid;
    enum rio_transaction transaction;
    uint8_t size;
    uint8_t status;
    uint32_t config_offset;
    unsigned payload_len;

    /* could do variable len tail array ([0]) and use obj allocator */
    uint64_t payload[RIO_MAX_PAYLOAD_SIZE];
};

void rio_print_pkt(struct rio_pkt *pkt);

struct rio_ep *rio_ep_create(const char *name, volatile uint32_t *base);
int rio_ep_destroy(struct rio_ep *ep);
int rio_ep_sp_send(struct rio_ep *ep, struct rio_pkt *pkt);
int rio_ep_sp_recv(struct rio_ep *ep, struct rio_pkt *pkt);

#endif // RIO_H
