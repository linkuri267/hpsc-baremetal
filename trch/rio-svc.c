#include <stdint.h>
#include <stdbool.h>

#include "console.h"
#include "panic.h"
#include "nvic.h"
#include "rio-ep.h"
#include "rio-switch.h"

#include "hwinfo.h"
#include "mem-map.h"

#include "rio-svc.h"

/* Configurable parameters chosen by this application */
#define MSG_SEG_SIZE 16
#define TRANSPORT_TYPE RIO_TRANSPORT_DEV8
#define ADDR_WIDTH RIO_ADDR_WIDTH_34_BIT
#define CFG_SPACE_SIZE 0x400000 /* for this app, care about only part of 16MB */

/* Normally these are assigned by discovery routine in the driver */
#define RIO_DEVID_EP0 0x0
#define RIO_DEVID_EP1 0x1

/* External endpoint to be recheable via the given port */
#define RIO_DEVID_EP_EXT 0x2
#define RIO_EP_EXT_SWITCH_PORT 2

#if 0 /* TODO */
#define CFG_SPACE_BASE { 0x0, 0x0 }
static const rio_addr_t cfg_space_base = CFG_SPACE_BASE;
#endif

/* API design objective: force consumers to refer to service via explicit
 * handle to make dependencies easy to see (as opposed to global funcs).
 * 
 * In case of this particular service, we choose to expose the handles
 * to drivers, so consumer interacts with the driver directly, but other
 * services may choose to encapsulate their driver handles forcing the
 * consumer to go through the service methods only.
 */

static struct rio_svc svc_obj;
static bool initialized = false;

struct rio_svc *rio_svc_create(bool master)
{
    struct rio_svc *svc = &svc_obj; /* see note on API design above */

    printf("RIO: initialize\r\n");
    ASSERT(!initialized);

    nvic_int_enable(TRCH_IRQ__RIO_1);

    svc->sw = rio_switch_create("RIO_SW", /* local */ true, RIO_SWITCH_BASE);
    if (!svc->sw)
        goto fail_sw;

    /* Partition buffer memory evenly among the endpoints */
    const unsigned buf_mem_size = RIO_MEM_SIZE / 2;
    uint8_t *buf_mem_cpu = (uint8_t *)RIO_MEM_WIN_ADDR;
    rio_bus_addr_t buf_mem_ep = RIO_MEM_ADDR;

    svc->ep0 = rio_ep_create("RIO_EP0", RIO_EP0_BASE,
                        RIO_EP0_OUT_AS_BASE, RIO_OUT_AS_WIDTH,
                        TRANSPORT_TYPE, ADDR_WIDTH,
                        buf_mem_ep, buf_mem_cpu, buf_mem_size);
    if (!svc->ep0)
        goto fail_ep0;
    buf_mem_ep += buf_mem_size;
    buf_mem_cpu += buf_mem_size;

    svc->ep1 = rio_ep_create("RIO_EP1", RIO_EP1_BASE,
                        RIO_EP1_OUT_AS_BASE, RIO_OUT_AS_WIDTH,
                        TRANSPORT_TYPE, ADDR_WIDTH,
                        buf_mem_ep, buf_mem_cpu, buf_mem_size);
    if (!svc->ep1)
        goto fail_ep1;
    buf_mem_ep += buf_mem_size;
    buf_mem_cpu += buf_mem_size;

    if (master) {
        rio_ep_set_devid(svc->ep0, RIO_DEVID_EP0);
        rio_ep_set_devid(svc->ep1, RIO_DEVID_EP1);

        rio_switch_map_local(svc->sw, RIO_DEVID_EP0, RIO_MAPPING_TYPE_UNICAST,
                             (1 << RIO_EP0_SWITCH_PORT));
        rio_switch_map_local(svc->sw, RIO_DEVID_EP1, RIO_MAPPING_TYPE_UNICAST,
                             (1 << RIO_EP1_SWITCH_PORT));
        
        /* Discover algorithm would run here and assign Device IDs... */
    }
    initialized = true;
    return svc;

fail_ep1:
    rio_ep_destroy(svc->ep0);
fail_ep0:
    rio_switch_destroy(svc->sw);
fail_sw:
    nvic_int_disable(TRCH_IRQ__RIO_1);
    return NULL;
}

void rio_svc_destroy(struct rio_svc *svc)
{
    ASSERT(initialized);
    ASSERT(svc == &svc_obj);
    ASSERT(svc->ep0 && svc->ep1 && svc->sw);

    rio_switch_unmap_local(svc->sw, RIO_DEVID_EP0);
    rio_switch_unmap_local(svc->sw, RIO_DEVID_EP1);

    rio_ep_set_devid(svc->ep0, 0);
    rio_ep_set_devid(svc->ep1, 0);

    rio_ep_destroy(svc->ep1);
    rio_ep_destroy(svc->ep0);
    rio_switch_destroy(svc->sw);

    nvic_int_disable(TRCH_IRQ__RIO_1);
}
