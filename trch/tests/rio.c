#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "mem.h"
#include "hwinfo.h"
#include "mem-map.h"
#include "nvic.h"
#include "dma.h"
#include "mmus.h"
#include "bit.h"
#include "sleep.h"
#include "rio-ep.h"
#include "rio-switch.h"
#include "syscfg.h"
#include "test.h"

/* Configurable parameters chosen by this test */
#define MSG_SEG_SIZE 16
#define TRANSPORT_TYPE RIO_TRANSPORT_DEV8
#define ADDR_WIDTH RIO_ADDR_WIDTH_34_BIT
#define CFG_SPACE_SIZE 0x400000 /* for this test, care about only part of 16MB */

/* Normally these are assigned by discovery routine in the driver */
#define RIO_DEVID_EP0 0x0
#define RIO_DEVID_EP1 0x1

/* External endpoint to be recheable via the given port */
#define RIO_DEVID_EP_EXT 0x2
#define RIO_EP_EXT_SWITCH_PORT 2

#define CFG_SPACE_BASE { 0x0, 0x0 }
static const rio_addr_t cfg_space_base = CFG_SPACE_BASE;

/* Read-only Device Identity CAR, not to be confused with assigned ID */
#define RIO_EP_DEV_ID   0x44332211 /* TODO: check real HW and update model */

/* The BM memory map (mem-map.h) assigns this test code some memory and some
 * MMU window space.  We partition them into sub-regions here. */

#define TEST_BUF_ADDR           (RIO_MEM_TEST_ADDR + 0x0)
#define TEST_BUF_SIZE           RIO_MEM_TEST_SIZE

/* to test data buffer in high mem */
#define TEST_BUF_WIN_ADDR     (RIO_TEST_WIN_ADDR + 0x000000) /* 0x20000 */
/* to EP0 outgoing map regions */
#define TEST_OUT_BUF_WIN_ADDR (RIO_TEST_WIN_ADDR + 0x020000) /* 0x20000 */
#define TEST_OUT_CFG_WIN_ADDR (RIO_TEST_WIN_ADDR + 0x040000) /* 0x400000 */
#define TEST_WIN_USED_SIZE                         0x440000

#if TEST_WIN_USED_SIZE < RIO_TEST_WIN_SIZE
#error Window region allocated for test is too small (RIO_TEST_WIN_SIZE)
#endif

/* This one happens to be defined in mem map, so cross-check.
   We don't use this size macro directly in favor of literal offsets, for
   legibility of the above partitioning. */
#if TEST_OUT_WIN_ADDR - TEST_BUF_WIN_ADDR > RIO_MEM_TEST_SIZE
#error Window partition for test buf smaller than RIO_MEM_TEST_SIZE
#endif

#define STR_(s) #s
#define STR(s) STR_(s)

static struct RioPkt in_pkt, out_pkt;
static uint8_t test_buf[RIO_MEM_TEST_SIZE]
    __attribute__((aligned (DMA_MAX_BURST_BYTES)));

static bool cmp_buf(uint8_t *p1, uint8_t *p2, unsigned size)
{
    for (int i = 0; i < size; ++i) {
        if (p1[i] != p2[i]) {
            return false;
        }
    }
    return true;
}

static void print_buf(uint8_t *p, unsigned size)
{
    for (int i = 0; i < size; ++i) {
        printf("%02x ", p[i]);
    }
    printf("\r\n");
}

static int test_send_receive(struct rio_ep *ep_from, struct rio_ep *ep_to)
{
    int rc = 1;

    printf("RIO TEST: %s: running send/receive test %s->%s...\r\n",
           __func__, rio_ep_name(ep_from), rio_ep_name(ep_to));

    bzero(&out_pkt, sizeof(out_pkt));

    out_pkt.addr_width = ADDR_WIDTH;
    out_pkt.ttype = TRANSPORT_TYPE;

    out_pkt.src_id = rio_ep_get_devid(ep_from);
    out_pkt.dest_id = rio_ep_get_devid(ep_to);

    /* Choose a packet type that we know is not auto-handled by HW */
    out_pkt.ftype = RIO_FTYPE_MAINT;
    out_pkt.target_tid = 0x0;
    out_pkt.hop_count = 0xFF; /* not destined to any switch */
    out_pkt.transaction = RIO_TRANS_MAINT_RESP_READ;

    out_pkt.status = 0x1;
    out_pkt.payload_len = 1;
    out_pkt.payload[0] = 0x01020304deadbeef;

    printf("RIO TEST: sending pkt on EP %s:\r\n", rio_ep_name(ep_from));
    rio_print_pkt(&out_pkt);

    rc = rio_ep_sp_send(ep_from, &out_pkt);
    if (rc) goto exit;

    rc = rio_ep_sp_recv(ep_to, &in_pkt);
    if (rc) goto exit;

    printf("RIO TEST: received pkt on EP %s:\r\n", rio_ep_name(ep_to));
    rio_print_pkt(&in_pkt);

    /* TODO: receive packets until expected response or timeout,
     * instead of blocking on receive and checking the first pkt */
    if (!(in_pkt.ftype == RIO_FTYPE_MAINT &&
          in_pkt.transaction == out_pkt.transaction &&
          in_pkt.target_tid == out_pkt.target_tid &&
          in_pkt.payload_len == 1 && in_pkt.payload[0] == out_pkt.payload[0])) {
        printf("RIO TEST: ERROR: receive packet does not match sent packet\r\n");
        goto exit;
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_read_csr(struct rio_ep *ep, rio_dest_t dest)
{
    uint32_t dev_id;
    uint32_t expected_dev_id = RIO_EP_DEV_ID;
    int rc = 1;

    printf("RIO TEST: running read CSR test...\r\n");

    rc = rio_ep_read_csr32(ep, &dev_id, dest, DEV_ID);
    if (rc) goto exit;

    if (dev_id != expected_dev_id) {
        printf("RIO TEST EP %s: unexpected value of "
                "DEV_ID CSR in EP (0x%x, %u): %x (expected %x)\r\n",
                rio_ep_name(ep), dest.devid, dest.hops, expected_dev_id);
        rc = 1;
        goto exit;
    }

    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int wr_csr_field(struct rio_ep *ep, rio_dest_t dest,
                        uint32_t csr, unsigned offset,
                        uint8_t *bytes, unsigned len,
                        uint32_t csr_val)
{
    int rc = 1;
    unsigned b;
    uint32_t read_val;
    uint8_t in_bytes[sizeof(uint32_t)] = {0};

    ASSERT(len < sizeof(in_bytes));

    rc = rio_ep_write_csr(ep, bytes, len, dest, csr + offset);
    if (rc) goto exit;

    rc = rio_ep_read_csr32(ep, &read_val, dest, csr);
    if (rc) goto exit;

    if (read_val != csr_val) {
        printf("RIO TEST: FAILED: csr 0x%x on devid %u not of expected value: "
               "%08x != %08x\r\n", csr, dest, read_val, csr_val);
        goto exit;
    }

    rc = rio_ep_read_csr(ep, in_bytes, len, dest, csr + offset);
    if (rc) goto exit;

    for (b = 0; b < len; ++b) {
        if (in_bytes[b] != bytes[b]) {
            printf("RIO TEST: FAIL: csr 0x%x field %u:%u byte %u W/R mismatch:"
                   "%x != %x\r\n", csr, offset, len, b,
                    in_bytes[b], bytes[b]);
            goto exit;
        }
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

/* Write/read in size smaller than dword and smaller than word (bytes) */
static int test_write_csr(struct rio_ep *ep, rio_dest_t dest)
{
    uint32_t csr = COMP_TAG;

    /* For the purposes of this test, we partition the csr word value into:
     * (bit numbering convention: 0 is MSB, as in the datasheet):
     *     0 - 15: short "s"
     *    16 - 23: byte "b"
     *    24 - 31: nothing
     */
    const uint16_t b_shift =  8;
    const uint16_t s_shift = 16;

    /* Four separate trials, using 3 sets of field values. */
    uint8_t val_b1  = 0xaa;
    uint8_t val_b2  = 0xdd;
    uint8_t val_b3  = 0x99;
    uint16_t val_s1 = 0xccbb;
    uint16_t val_s2 = 0xffee;
    uint16_t val_s3 = 0x8877;

    int rc = 1;
    uint32_t reg = 0, read_reg, orig_reg;
    uint8_t bytes[sizeof(uint32_t)] = {0};

    printf("RIO TEST: running write CSR test...\r\n");

    printf("RIO TEST: write csr: saving CSR value\r\n");
    rc = rio_ep_read_csr32(ep, &orig_reg, dest, csr);
    if (rc) goto exit;

    printf("RIO TEST: write csr: testing whole CSR\r\n");
    reg = ((uint32_t)val_b1 << b_shift) | ((uint32_t)val_s1 << s_shift);

    rc = rio_ep_write_csr32(ep, reg, dest, csr);
    if (rc) goto exit;

    rc = rio_ep_read_csr32(ep, &read_reg, dest, csr);
    if (rc) goto exit;

    if (read_reg != reg) {
        printf("RIO TEST: FAILED: csr 0x%x on devid %u not of expected value: "
               "%08x != %08x\n", csr, dest, read_reg, reg);
        goto exit;
    }

    printf("RIO TEST: write csr: testing field: byte\r\n");
    reg &= ~(0xff << b_shift);
    reg |= (uint32_t)val_b2 << b_shift;
    bytes[0] = val_b2;
    rc = wr_csr_field(ep, dest, csr, /* offset */ b_shift / 8,
                      bytes, sizeof(val_b2), reg);
    if (rc) goto exit;

    printf("RIO TEST: write csr: testing field: short\r\n");
    reg &= ~(0xffff << s_shift);
    reg |= val_s2 << s_shift;
    bytes[0] = val_s2 >> 8;
    bytes[1] = val_s2 & 0xff;
    rc = wr_csr_field(ep, dest, csr, /* offset */ s_shift / 8,
                      bytes, sizeof(val_s2), reg);
    if (rc) goto exit;

    printf("RIO TEST: write csr: testing fields: byte and short\r\n");
    reg = ((uint32_t)val_b3 << b_shift) | ((uint32_t)val_s3 << s_shift);
    bytes[0] = val_s3 >> 8;
    bytes[1] = val_s3 & 0xff;
    bytes[2] = val_b3;
    rc = wr_csr_field(ep, dest, csr, /* offset */ b_shift / 8,
                      bytes, sizeof(val_b3) + sizeof(val_s3), reg);
    if (rc) goto exit;

    printf("RIO TEST: write csr: restoring CSR value\r\n");
    rc = rio_ep_read_csr32(ep, &orig_reg, dest, csr);
    if (rc) goto exit;

    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_msg(struct rio_ep *ep0, struct rio_ep *ep1)
{
    const rio_devid_t dest = RIO_DEVID_EP1;
    const unsigned mbox = 0;
    const unsigned letter = 0;
    const unsigned seg_size = 8;
    const unsigned launch_time = 0;
    uint64_t payload = 0x01020304deadbeef;

    int rc = 1;

    printf("RIO TEST: running message test...\r\n");

    rc = rio_ep_msg_send(ep0, dest, launch_time, mbox, letter, seg_size,
                         (uint8_t *)&payload, sizeof(payload));
    if (rc) goto exit;

    rio_devid_t src_id = 0;
    uint64_t rcv_time = 0;
    uint64_t rx_payload = 0;
    unsigned payload_len = sizeof(rx_payload);
    rc = rio_ep_msg_recv(ep1, mbox, letter, &src_id, &rcv_time,
                         (uint8_t *)&rx_payload, &payload_len);
    if (rc) goto exit;

    printf("RIO TEST: recved msg from %u at %08x%08x payload len %u %08x%08x\r\n",
           src_id, (uint32_t)(rcv_time >> 32), (uint32_t)(rcv_time & 0xffffffff),
           payload_len,
           (uint32_t)(rx_payload >> 32), (uint32_t)(rx_payload & 0xffffffff));

    if (rx_payload != payload) {
        printf("RIO TEST: ERROR: received msg payload mismatches sent\r\n");
        goto exit;
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_doorbell(struct rio_ep *ep0, struct rio_ep *ep1)
{
    int rc = 1;
    enum rio_status status;

    printf("RIO TEST: running doorbell test...\r\n");

    uint16_t info = 0xf00d, info_recv = 0;
    rc = rio_ep_doorbell_send_async(ep0, RIO_DEVID_EP1, info);
    if (rc) goto exit;
    rc = rio_ep_doorbell_recv(ep1, &info_recv);
    if (rc) goto exit;
    rc = rio_ep_doorbell_reap(ep0, &status);
    if (rc) goto exit;

    if (status != RIO_STATUS_DONE) {
        printf("RIO TEST: FAILED: doorbell request returned error: "
               "status 0x%x\n\n", status);
        goto exit;
    }
    if (info_recv != info) {
        printf("RIO TEST: FAILED: received doorbell has mismatched info: "
               "0x%x != 0x%x\r\n", info_recv, info);
        goto exit;
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static uint64_t uint_from_buf(uint8_t *buf, unsigned bytes)
{
    uint64_t v = 0;
    for (int b = 0; b < bytes; ++b) {
        v <<= 8;
        v |= buf[bytes - b - 1];
    }
    return v;
}

static bool check_equality(uint64_t val, uint64_t ref)
{
    if (val != ref) {
        printf("TEST: RIO map: value mismatch: 0x%08x%08x != 0x%08x%08x\r\n",
                (uint32_t)(val >> 32), (uint32_t)(val & 0xffffffff),
                (uint32_t)(ref >> 32), (uint32_t)(ref & 0xffffffff));
        return false;
    }
    return true;
}

struct map_target_info {
    const char *name;
    uint32_t size;
    /* Address of outgoing region in EP0's system address space */
    uint64_t out_addr;
    /* 32-bit system bus addr of window into out_addr translated by MMU */
    uint8_t *window_addr;
    /* Address of target region on the RapidIO interconnect */
    rio_addr_t rio_addr;
    bool bus_window; /* whether to create a window into bus_addr */
    /* Address the remote endpoint uses on its system bus for the target */
    rio_bus_addr_t bus_addr; /* optional */
    /* 32-bit system bus addr of window into bus_addr translated by MMU */
    uint8_t *bus_win_addr; /* optional */
};


enum {
    OUT_REGION_CFG_SPACE = 0,
    OUT_REGION_TEST_BUF,
    NUM_OUT_REGIONS
};

/* Group region metadata to be able to loop over them below */
static struct map_target_info map_targets[] = {
    [OUT_REGION_CFG_SPACE] = {
        .name = STR(OUT_REGION_CFG_SPACE),
        .size = CFG_SPACE_SIZE,
        .window_addr = (uint8_t *)TEST_OUT_CFG_WIN_ADDR,
        .rio_addr = CFG_SPACE_BASE,
        .bus_window = false,
        .bus_addr = 0, /* unset */
    },
    [OUT_REGION_TEST_BUF] = {
        .name = STR(OUT_REGION_TEST_BUF),
        .size = RIO_MEM_TEST_SIZE,
        .window_addr = (uint8_t *)TEST_OUT_BUF_WIN_ADDR,
        .rio_addr = { 0, TEST_BUF_ADDR & ALIGN64_MASK(ADDR_WIDTH) },
        .bus_addr = TEST_BUF_ADDR,
        .bus_window = true,
        .bus_win_addr = (uint8_t *)TEST_BUF_WIN_ADDR,
    },
};

static rio_addr_t saved_cfg_base;

static int map_setup(struct rio_ep *ep0, struct rio_ep *ep1)
{
    int rc = 1;

    /* CSR space is always mapped into incoming region; set the location
     * explicitly to not rely on default value upon reset. */
    saved_cfg_base = rio_ep_get_cfg_base(ep1);
    rio_ep_set_cfg_base(ep1, cfg_space_base);

    /* The endpoint determines which region config to use exclusively
     * based on source device ID, and not based on address. TODO: true? */
    enum {
        IN_ADDR_SPACE_FROM_EP0 = 0,
        NUM_IN_ADDR_SPACES
    };
    struct rio_map_in_as_cfg in_as_cfgs[] = {
        [IN_ADDR_SPACE_FROM_EP0] = {
            .src_id = RIO_DEVID_EP0,
            .src_id_mask = 0xff,
            .addr_width = ADDR_WIDTH,

            /* Configuring for the test memory region to be accessible;
               if you want additional accessibility outside this region,
               then the only way is to increase RIO address width.
               These bus address high bits are only relevant when bus
               address is wider than RIO address (e.g. bus is 40-bit, RIO is
               34-bit. TODO: <--- is this how the real HW works. */
            .bus_addr = RIO_MEM_TEST_ADDR & ~ALIGN64_MASK(ADDR_WIDTH),
        },
    };
    ASSERT(sizeof(in_as_cfgs) / sizeof(in_as_cfgs[0]) == NUM_IN_ADDR_SPACES);

    /* The RIO address is split into: <hi bits> | <within region offset> */
    int out_region_size_bits =
        rio_ep_get_outgoing_size_bits(ep0, NUM_OUT_REGIONS);

    struct rio_map_out_region_cfg out_region_cfgs[NUM_OUT_REGIONS];

    for (int i = 0; i < NUM_OUT_REGIONS; ++i) {
        struct map_target_info *tgt = &map_targets[i];

        /* LCS register maps the configuration space (CSR) into the incoming
         * address space, so make sure our buffer does not overlap. In other
         * words, the range [0..16MB] of the system bus address space cannot be
         * accessed via RapidIO requests. */
        rio_addr_t cfg_space_bound =
            rio_addr_add64(cfg_space_base, RIO_CSR_REGION_SIZE);
        if (i != OUT_REGION_CFG_SPACE &&
            rio_addr_cmp(cfg_space_base, tgt->rio_addr) <= 0 &&
            rio_addr_cmp(tgt->rio_addr, cfg_space_bound) < 0) {
            printf("RIO TEST: ERROR: target %s overlaps with RapidIO CSR region"
                   " ([0..0x%x]): tgt 0x%02x%08x%08x cfg 0x%02x%08x%08x\r\n",
                   tgt->name, RIO_CSR_REGION_SIZE,
                   tgt->rio_addr.hi,
                   (uint32_t)(tgt->rio_addr.lo >> 32),
                   (uint32_t)tgt->rio_addr.lo,
                   cfg_space_base.hi,
                   (uint32_t)(cfg_space_base.lo >> 32),
                   (uint32_t)cfg_space_base.lo);
            return 1;
        }

        struct rio_map_in_as_cfg *in_cfg = &in_as_cfgs[IN_ADDR_SPACE_FROM_EP0];
        rio_bus_addr_t bus_addr = tgt->bus_addr & ~ALIGN64_MASK(ADDR_WIDTH);
        if (bus_addr != in_cfg->bus_addr) {
            printf("RIO TEST: ERROR: "
                   "target %s not addressable via incoming region: "
                   "bus addr (hibits): trgt %0808x != incoming %08x%08x\r\n",
                   tgt->name, (uint32_t)(bus_addr >> 32), (uint32_t)bus_addr,
                   (uint32_t)(in_cfg->bus_addr>> 32),
                   (uint32_t)in_cfg->bus_addr);
            return 1;
        }

        ASSERT(out_region_size_bits <= sizeof(tgt->rio_addr.lo) * 8);
        tgt->out_addr = rio_ep_get_outgoing_base(ep0, NUM_OUT_REGIONS, i) +
                (tgt->rio_addr.lo & ALIGN64_MASK(out_region_size_bits));

        struct rio_map_out_region_cfg *reg_cfg = &out_region_cfgs[i];
        reg_cfg->ttype = TRANSPORT_TYPE;
        reg_cfg->dest_id = RIO_DEVID_EP1;
        reg_cfg->addr_width = in_cfg->addr_width;

        ASSERT(out_region_size_bits <= sizeof(tgt->rio_addr.lo) * 8);
        rio_addr_t out_rio_addr = {
            tgt->rio_addr.hi,
            tgt->rio_addr.lo & ~ALIGN64_MASK(out_region_size_bits)
        };
        reg_cfg->rio_addr = out_rio_addr;
    }

    rc = rio_ep_map_in(ep1, NUM_IN_ADDR_SPACES, in_as_cfgs);
    if (rc) goto fail_map_in;

    rc = rio_ep_map_out(ep0, NUM_OUT_REGIONS, out_region_cfgs);
    if (rc) goto fail_map_out;

    for (int i = 0; i < NUM_OUT_REGIONS; ++i) {
        struct map_target_info *tgt = &map_targets[i];

        if (tgt->bus_window) {
            rc = rt_mmu_map((uintptr_t)tgt->bus_win_addr, tgt->bus_addr,
                            tgt->size);
            if (rc) goto cleanup;
        } else {
            tgt->bus_win_addr = NULL;
        }

        rc = rt_mmu_map((uintptr_t)tgt->window_addr, tgt->out_addr, tgt->size);
        if (rc) {
            if (tgt->bus_window)
                rt_mmu_unmap((uintptr_t)map_targets[i].bus_win_addr,
                             map_targets[i].size);
            goto cleanup;
        }

        continue;
cleanup:
        for (--i; i >= 0; --i) {
            rt_mmu_unmap((uintptr_t)map_targets[i].window_addr,
                         map_targets[i].size);
            if (tgt->bus_window)
                rt_mmu_unmap((uintptr_t)map_targets[i].bus_win_addr,
                             map_targets[i].size);
        }
        goto fail_map_target;
    }

    printf("RIO TEST: mapped targets:\r\n");
    for (int i = 0; i < NUM_OUT_REGIONS; ++i) {
        struct map_target_info *tgt = &map_targets[i];
        printf("\tTarget %s:\r\n"
               "\t\t  on RIO interconnect: %02x%08x%08x\r\n"
               "\t\tvia RIO to EP1 to mem:   %08x%08x [window: %p]\r\n"
               "\t\t       via system bus:   %08x%08x [window: %p]\r\n",
                tgt->name,
                tgt->rio_addr.hi,
                (uint32_t)(tgt->rio_addr.lo >> 32), (uint32_t)tgt->rio_addr.lo,
                (uint32_t)(tgt->out_addr >> 32), (uint32_t)tgt->out_addr,
                tgt->window_addr,
                (uint32_t)(tgt->bus_addr >> 32), (uint32_t)tgt->bus_addr,
                tgt->bus_win_addr);
    }
    rc = 0;
    goto exit;

fail_map_target:
    rio_ep_unmap_out(ep0);
fail_map_out:
    rio_ep_unmap_in(ep1);
fail_map_in:
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static void map_teardown(struct rio_ep *ep0, struct rio_ep *ep1)
{
    for (int i = 0; i < NUM_OUT_REGIONS; ++i) {
        struct map_target_info *tgt = &map_targets[i];
        rt_mmu_unmap((uintptr_t)tgt->window_addr, tgt->size);
        if (tgt->bus_window)
            rt_mmu_unmap((uintptr_t)tgt->bus_win_addr, tgt->size);
    }

    rio_ep_unmap_out(ep0);
    rio_ep_unmap_in(ep1);

    rio_ep_set_cfg_base(ep1, saved_cfg_base);
}

static int test_map_single(uint8_t *sys_addr, uint8_t *out_addr,
                           unsigned mem_size)
{
    int rc = 1;

    printf("RIO TEST: running map single test...\r\n");

    int i, j, k;
    uint8_t test_dw[] = { 0xA1, 0xB2, 0xC3, 0xD4, 0xA5, 0xB6, 0xC7, 0xD8 };

/* The test buf contains many double-words; choose how many of them to access */
#define DWORDS_TO_TEST 2
    
    ASSERT(DWORDS_TO_TEST * sizeof(uint64_t) <= mem_size);

    for (j = 0; j < DWORDS_TO_TEST; ++j) {
        uint8_t *dw_sys_addr = sys_addr + j * sizeof(uint64_t);
        uint8_t *dw_out_addr = out_addr + j * sizeof(uint64_t);

        /* Note: When this code runs on a 32-bit processor, the 64-bit access
         * will be broken up into two 32-bit accesses by the compiler. */
        unsigned sizes[] = { sizeof(uint8_t), sizeof(uint16_t),
                             sizeof(uint32_t), sizeof(uint64_t) };
        for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            unsigned size = sizes[i];
            for (k = 0; k < sizeof(uint64_t) /* dword */; k += size) {

                uint8_t *granule_out_addr = (uint8_t *)dw_out_addr + k;
                uint8_t *granule_sys_addr = (uint8_t *)dw_sys_addr + k;

                uint64_t ref, val;

                /* Write test_dw[0:size] directly to mem, read via RIO */
                bzero(dw_sys_addr, sizeof(uint64_t));
                memcpy(granule_sys_addr, test_dw, size);
                ref = uint_from_buf(granule_sys_addr, size);
                printf("RIO TEST: testing read  of size %u bytes: "
                       "@%x ref %08x%08x\r\n", size, granule_out_addr,
                       (uint32_t)(ref >> 32), (uint32_t)ref);
                switch (size) {
                    case sizeof(uint8_t):
                        val = *((uint8_t *)(granule_out_addr));
                        break;
                    case sizeof(uint16_t):
                        val = *((uint16_t *)(granule_out_addr));
                        break;
                    case sizeof(uint32_t):
                        val = *((uint32_t *)(granule_out_addr));
                        break;
                    case sizeof(uint64_t):
                        val = *((uint64_t *)(granule_out_addr));
                        break;
                }
                if (!check_equality(val, ref))
                    goto exit;

                /* Write test_dw[0:size] via RIO, read directly from mem */
                bzero(dw_sys_addr, sizeof(uint64_t));
                ref = uint_from_buf(test_dw, size);
                printf("RIO TEST: testing write of size %u bytes: "
                       "@%x ref %08x%08x\r\n", size, granule_out_addr,
                       (uint32_t)(ref >> 32), (uint32_t)ref);
                switch (size) {
                    case sizeof(uint8_t):
                        *((uint8_t *)granule_out_addr) = (uint8_t)ref;
                        break;
                    case sizeof(uint16_t):
                        *((uint16_t *)granule_out_addr) = (uint16_t)ref;
                        break;
                    case sizeof(uint32_t):
                        *((uint32_t *)granule_out_addr) = (uint32_t)ref;
                        break;
                    case sizeof(uint64_t):
                        *((uint64_t *)granule_out_addr) = (uint64_t)ref;
                        break;
                }
                val = uint_from_buf(granule_sys_addr, size);
                if (!check_equality(val, ref))
                    goto exit;
            }

        /* Test misaligned access */
#if 0 /* TODO: expect fault */
        uint16_t valx = *((uint16_t *)dw_out_addr + 1); 
#endif
        }
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_map_burst(uint8_t *sys_addr, uint8_t *out_addr,
                          unsigned mem_size, struct dma *dmac)
{
    int rc = 1;

    printf("RIO TEST: running map burst test...\r\n");

    /* fill destination with test data via direct local access */
    for (int i = 0; i < mem_size; ++i) {
        sys_addr[i] = i;
    }

    ASSERT(ALIGNED(mem_size, DMA_MAX_BURST_BITS));
    ASSERT(ALIGNED(&test_buf[0], DMA_MAX_BURST_BITS));
    ASSERT(ALIGNED(sys_addr, DMA_MAX_BURST_BITS));
    ASSERT(ALIGNED(out_addr, DMA_MAX_BURST_BITS));

    /* TODO: allocate channel per user */
    unsigned dma_chan = 0;

    /* DMA from mapped region into local buffer */

    /* TODO: use HPPS SRIO DMA */
    struct dma_tx *dtx = dma_transfer(dmac, dma_chan,
        (uint32_t *)out_addr, (uint32_t *)test_buf, mem_size,
         /* no callback */ NULL, NULL);
    rc = dma_wait(dtx);
    if (rc) {
        printf("TEST RIO map DMA: transfer from region failed: rc %u\r\n", rc);
        goto exit;
    }

    const int buf_preview_len = 16;

    printf("test_buf: "); print_buf(test_buf, buf_preview_len);
    printf("sys_addr: "); print_buf(sys_addr, buf_preview_len);
    if (!cmp_buf(test_buf, sys_addr, mem_size)) {
        printf("TEST RIO map DMA: mismatch in read data\r\n");
        goto exit;
    }

    /* clear destination */
    bzero(sys_addr, mem_size);

    /* DMA from local buffer into mapped region */

    dtx = dma_transfer(dmac, dma_chan,
        (uint32_t *)test_buf, (uint32_t *)out_addr, mem_size,
        /* no callback */ NULL, NULL);
    rc = dma_wait(dtx);
    if (rc) {
        printf("TEST RIO map DMA: transfer to map region failed: rc %u\r\n", rc);
        goto exit;
    }

    /* compare destination to local buf via direct access to destination */
    printf("test_buf: "); print_buf(test_buf, buf_preview_len);
    printf("sys_addr: "); print_buf(sys_addr, buf_preview_len);
    if (!cmp_buf(test_buf, sys_addr, mem_size)) {
        printf("TEST RIO map DMA: mismatch in written data\r\n");
        goto exit;
    }
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_rw_cfg_space(struct rio_ep *ep, rio_devid_t dest)
{
    int rc = 1;
    uint32_t csr_addr; /* 16MB range */
    rio_addr_t csr_rio_addr; /* up to 66-bit RapidIO address */
    uint32_t csr_ref, csr_val, csr_saved;

    printf("RIO TEST: running read/write cfg space test...\r\n");

    csr_addr = DEV_ID;
    csr_rio_addr = rio_addr_from_u64(csr_addr);
    csr_ref = RIO_EP_DEV_ID;
    csr_val = 0;

    printf("RIO TEST: read cfg space at addr 0x%08x\r\n", csr_addr);
    rc = rio_ep_read32(ep, &csr_val, csr_rio_addr, dest);
    if (rc) goto exit;

    if (csr_val != csr_ref) {
        printf("RIO TEST: ERROR: read CSR value mismatches expected: "
               "0x%x != 0x%x\r\n", csr_val, csr_ref);
        goto exit;
    }

    csr_addr = COMP_TAG;
    csr_rio_addr = rio_addr_from_u64(csr_addr);
    csr_ref = 0xd00df00d;
    csr_val = 0;

    printf("RIO TEST: save/write/read/restore cfg space at addr 0x%08x\r\n",
           csr_addr);
    rc = rio_ep_read32(ep, &csr_saved, csr_rio_addr, dest);
    if (rc) goto exit;
    rc = rio_ep_write32(ep, csr_ref, csr_rio_addr, dest);
    if (rc) goto exit;
    rc = rio_ep_read32(ep, &csr_val, csr_rio_addr, dest);
    if (rc) goto exit;
    if (csr_val != csr_ref) {
        printf("RIO TEST: ERROR: read CSR value mismatches expected: "
               "0x%x != 0x%x\r\n", csr_val, csr_ref);
        goto exit;
    }
    rc = rio_ep_write32(ep, csr_saved, csr_rio_addr, dest);
    if (rc) goto exit;

    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

static int test_map_rw_cfg_space(uint32_t *csr_out_addr)
{
    int rc = 1;
    uint32_t csr_addr; /* 16MB range */
    uint32_t *csr_ptr; /* pointer via outgoing region */
    uint32_t csr_ref, csr_val, csr_saved;

    printf("RIO TEST: running read/write cfg space via mapped region...\r\n");

    csr_addr = DEV_ID;
    csr_ptr = (uint32_t *)((uint8_t *)csr_out_addr + csr_addr);
    csr_ref = RIO_EP_DEV_ID;

    printf("RIO TEST: read CSR 0x%x at addr %p\r\n", csr_addr, csr_ptr);
    csr_val = *csr_ptr;
    if (csr_val != csr_ref) {
        printf("RIO TEST: ERROR: read CSR value mismatches expected: "
               "0x%x != 0x%x\r\n", csr_val, csr_ref);
        goto exit;
    }

    csr_addr = COMP_TAG;
    csr_ptr = (uint32_t *)((uint8_t *)csr_out_addr + csr_addr);
    csr_ref = 0xd00df00d;
    csr_val = 0;

    printf("RIO TEST: save/write/read/restore CSR 0x%x at addr %p\r\n",
           csr_addr, csr_ptr);
    csr_saved = *csr_ptr;
    *csr_ptr = csr_ref;
    csr_val = *csr_ptr;
    if (csr_val != csr_ref) {
        printf("RIO TEST: ERROR: CSR 0x%x mismatches written value: "
               "0x%x != 0x%x\r\n", csr_addr, csr_val, csr_ref);
        goto exit;
    }
    *csr_ptr = csr_saved;
    rc = 0;
exit:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

int test_hop_routing(struct rio_ep *ep0, struct rio_ep *ep1)
{
    int rc = 1;

/* We don't do enumeration/discovery traversal, hence hop-based test is limited
 * (hop-based routing with multiple hops requires device IDs to be assigned
 * along the way on all but the last hop). */
#if (RIO_HOPS_FROM_EP0_TO_SWITCH > 0) || (RIO_HOPS_FROM_EP1_TO_SWITCH > 0)
#error RIO hop-based routing test supports only hop-count 0.
#endif

    rio_dest_t ep0_to_switch = rio_sw_dest(0, RIO_HOPS_FROM_EP0_TO_SWITCH);

    rc = rio_switch_map_remote(ep0, ep0_to_switch, RIO_DEVID_EP0,
            RIO_MAPPING_TYPE_UNICAST, (1 << RIO_EP0_SWITCH_PORT));
    if (rc) goto exit_0;

    rc = rio_switch_map_remote(ep0, ep0_to_switch, RIO_DEVID_EP1,
            RIO_MAPPING_TYPE_UNICAST, (1 << RIO_EP1_SWITCH_PORT));
    if (rc) goto exit_1;

    rc = test_send_receive(ep0, ep1);
    if (rc) goto exit;
    rc = test_send_receive(ep1, ep0);
    if (rc) goto exit;

    rc = 0;
exit:
    rc |= rio_switch_unmap_remote(ep0, ep0_to_switch, RIO_DEVID_EP0);
exit_1:
    rc |= rio_switch_unmap_remote(ep0, ep0_to_switch, RIO_DEVID_EP1);
exit_0:
    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

/* To test the backend, have to talk to another Qemu instance or a standalone
 * device model. Can't test the out-of-process backend using loopback alone
 * (e.g. send from EP0 to EP1 via external path), because loopback out of the
 * switch back into the same switch creates an infinite routing loop (would
 * need to somehow change the routing table in the middle of the test).
 * Alternatively, could make a proxy that rewrites destination ID, which could
 * be a standalone process or within Qemu process (slightly weaker test). */
int test_backend(struct rio_switch *sw, struct rio_ep *ep)
{
    int rc = 1;

    printf("RIO TEST: %s: running backend test...\r\n", __func__);

    rio_switch_map_local(sw, RIO_DEVID_EP_EXT, RIO_MAPPING_TYPE_UNICAST,
                          (1 << RIO_EP_EXT_SWITCH_PORT));

    rio_dest_t ep_ext = rio_dev_dest(RIO_DEVID_EP_EXT);
    rio_dest_t ep_to_ext_switch = rio_sw_dest(RIO_DEVID_EP_EXT, /* hops */ 1);

    rc = rio_switch_map_remote(ep, ep_to_ext_switch, RIO_DEVID_EP0,
            RIO_MAPPING_TYPE_UNICAST, (1 << RIO_EP_EXT_SWITCH_PORT));
    if (rc) goto exit;

    rc = rio_switch_map_remote(ep, ep_to_ext_switch, RIO_DEVID_EP_EXT,
            RIO_MAPPING_TYPE_UNICAST, (1 << RIO_EP0_SWITCH_PORT));
    if (rc) goto exit;

    printf("RIO TEST: %s: setting dev ID of remote EP...\r\n", __func__);

    /* Device ID in our packet won't match the devid of EXT EP, but the EP
     * should process the request regardless (by spec). */
    uint32_t did = 0;
    did = FIELD_DP32(did, B_DEV_ID, BASE_DEVICE_ID, RIO_DEVID_EP_EXT);
    did = FIELD_DP32(did, B_DEV_ID, LARGE_BASE_DEVICE_ID, RIO_DEVID_EP_EXT);
    rc = rio_ep_write_csr32(ep, did, ep_ext, B_DEV_ID);
    if (rc) goto exit;

    printf("RIO TEST: %s: read/write CSRs in remote EP...\r\n", __func__);

    rc = test_read_csr(ep, ep_ext);
    if (rc) goto exit;
    rc = test_write_csr(ep, ep_ext);
    if (rc) goto exit;

    /* TODO: test mapped regions */
    /* TODO: test message layer */

    rc = 0;
exit:
    rio_switch_unmap_local(sw, RIO_DEVID_EP_EXT);

    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "FAILED" : "PASSED");
    return rc;
}

int test_rio(unsigned instance, struct dma *dmac)
{
    int rc = 1;

    printf("RIO TEST: machine instance %u\r\n", instance);

    nvic_int_enable(TRCH_IRQ__RIO_1);

    struct rio_switch *sw = rio_switch_create("RIO_SW", /* local */ true,
                                              RIO_SWITCH_BASE);
    if (!sw)
        goto fail_sw;

    /* Partition buffer memory evenly among the endpoints */
    const unsigned buf_mem_size = RIO_MEM_SIZE / 2;
    uint8_t *buf_mem_cpu = (uint8_t *)RIO_MEM_WIN_ADDR;
    rio_bus_addr_t buf_mem_ep = RIO_MEM_ADDR;

    struct rio_ep *ep0 = rio_ep_create("RIO_EP0", RIO_EP0_BASE,
                                       RIO_EP0_OUT_AS_BASE, RIO_OUT_AS_WIDTH,
                                       TRANSPORT_TYPE, ADDR_WIDTH,
                                       buf_mem_ep, buf_mem_cpu, buf_mem_size);
    if (!ep0)
        goto fail_ep0;
    buf_mem_ep += buf_mem_size;
    buf_mem_cpu += buf_mem_size;

    struct rio_ep *ep1 = rio_ep_create("RIO_EP1", RIO_EP1_BASE,
                                       RIO_EP1_OUT_AS_BASE, RIO_OUT_AS_WIDTH,
                                       TRANSPORT_TYPE, ADDR_WIDTH,
                                       buf_mem_ep, buf_mem_cpu, buf_mem_size);
    if (!ep1)
        goto fail_ep1;
    buf_mem_ep += buf_mem_size;
    buf_mem_cpu += buf_mem_size;

    /* Device IDs needed for routing any requests in subsequent tests */

    /* Machine instance 1 needs to keep EPs initialized while letting HW
     * listen and process requests from machine 0 (transparent to SW). */
    if (instance == 1) {
        return 0; /* don't destroy anything (TODO: add RIO init to main.c? */
    }

    rc = rio_ep_set_devid(ep0, RIO_DEVID_EP0);
    if (rc) goto fail;
    rc = rio_ep_set_devid(ep1, RIO_DEVID_EP1);
    if (rc) goto fail;

    rio_switch_map_local(sw, RIO_DEVID_EP0, RIO_MAPPING_TYPE_UNICAST,
                         (1 << RIO_EP0_SWITCH_PORT));
    rio_switch_map_local(sw, RIO_DEVID_EP1, RIO_MAPPING_TYPE_UNICAST,
                         (1 << RIO_EP1_SWITCH_PORT));
    rio_switch_map_local(sw, RIO_DEVID_EP_EXT, RIO_MAPPING_TYPE_UNICAST,
                         (1 << RIO_EP_EXT_SWITCH_PORT));

    /* so that we can ifdef tests out without warnings */
    (void)test_send_receive;
    (void)test_read_csr;
    (void)test_write_csr;
    (void)test_msg;
    (void)test_doorbell;
    (void)test_map_single;
    (void)test_map_burst;
    (void)test_rw_cfg_space;
    (void)test_map_rw_cfg_space;
    (void)test_hop_routing;
    (void)test_backend;
    (void)map_setup;
    (void)map_teardown;

    rc = test_send_receive(ep0, ep1);
    if (rc) goto fail;
    rc = test_send_receive(ep1, ep0);
    if (rc) goto fail;

    rc = test_read_csr(ep0, rio_dev_dest(RIO_DEVID_EP1));
    if (rc) goto fail;
    rc = test_read_csr(ep1, rio_dev_dest(RIO_DEVID_EP0));
    if (rc) goto fail;

    rc = test_write_csr(ep0, rio_dev_dest(RIO_DEVID_EP1));
    if (rc) goto fail;
    rc = test_write_csr(ep1, rio_dev_dest(RIO_DEVID_EP0));
    if (rc) goto fail;

    rc = test_msg(ep0, ep1);
    if (rc) goto fail;
    rc = test_doorbell(ep0, ep1);
    if (rc) goto fail;

    rc = map_setup(ep0, ep1);
    if (rc) goto fail;

    struct map_target_info *buf_tgt = &map_targets[OUT_REGION_TEST_BUF];
    struct map_target_info *cfg_tgt = &map_targets[OUT_REGION_CFG_SPACE];

    rc = test_map_single(buf_tgt->bus_win_addr, buf_tgt->window_addr,
                         buf_tgt->size);
    if (rc) goto fail_map;
    rc = test_map_burst(buf_tgt->bus_win_addr, buf_tgt->window_addr,
                        buf_tgt->size, dmac);
    if (rc) goto fail_map;

    rc = test_map_rw_cfg_space((uint32_t *)cfg_tgt->window_addr);
    if (rc) goto fail_map;

    /* NOTE: EP1->EP0 won't work unless we setup incoming reg on EP0 */
    rc = test_rw_cfg_space(ep0, RIO_DEVID_EP1);
    if (rc) goto fail_map;

    /* Before hop routing test: reset switch routing table */
    rio_switch_unmap_local(sw, RIO_DEVID_EP0);
    rio_switch_unmap_local(sw, RIO_DEVID_EP1);

    rc = test_hop_routing(ep0, ep1);
    if (rc) goto fail;

    /* After hop routing test: restore switch routing table */
    rio_switch_map_local(sw, RIO_DEVID_EP0, RIO_MAPPING_TYPE_UNICAST,
                         (1 << RIO_EP0_SWITCH_PORT));
    rio_switch_map_local(sw, RIO_DEVID_EP1, RIO_MAPPING_TYPE_UNICAST,
                         (1 << RIO_EP1_SWITCH_PORT));

    rc = test_backend(sw, ep0);
    if (rc) goto fail;

    rc = 0;
fail_map:
    map_teardown(ep0, ep1);
fail:
    rio_ep_destroy(ep1);
fail_ep1:
    rio_ep_destroy(ep0);
fail_ep0:
    /* These unmaps may be redundant on some paths, but it's harmless */
    rio_switch_unmap_local(sw, RIO_DEVID_EP0);
    rio_switch_unmap_local(sw, RIO_DEVID_EP1);
    rio_switch_destroy(sw);
fail_sw:
    nvic_int_disable(TRCH_IRQ__RIO_1);

    printf("RIO TEST: %s: %s\r\n", __func__, rc ? "SOME FAILED!" : "ALL PASSED");
    return rc;
}

void rio_1_isr()
{
    nvic_int_disable(TRCH_IRQ__RIO_1);
}
