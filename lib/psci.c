
#include <stdint.h>
#include <unistd.h>

#include "command.h"
#include "pm_defs.h"
#include "printf.h"


struct pm_state {
    uint32_t power_state;
    uint32_t addr_low;
    uint32_t addr_high;
};

struct pm_state hpsc_nodes[PM_API_MAX];

static int pm_self_suspend(uint32_t sender, uint32_t * data)
{
    uint32_t target = data[0];
    uint32_t latency = data[1];
    uint32_t state = data[2];
    uint32_t addr_low = data[3];
    uint32_t addr_high = data[4];
    printf("%s: sender(%d), target(%d), latency(%d), state(%d), addr_low(0x%x), addr_high(0x%x)\r\n", __func__, 
		sender, target, latency, state, addr_low, addr_high);
    if (addr_low & 0x1) { /* save address */
        hpsc_nodes[target].addr_low = addr_low;
        hpsc_nodes[target].addr_high = addr_high;
    }
    return 0;
}

static int pm_req_suspend(uint32_t sender, uint32_t * data)
{
    uint32_t target = data[0];
    uint32_t ack = data[1];
    uint32_t latency = data[2];
    uint32_t state = data[3];
    printf("%s: not implemented yet: target(%d), ack(%d), latency(%d), state(%d)\r\n", __func__, 
		target, ack, latency, state);
    return 0;
}

static int pm_force_powerdown(uint32_t sender, uint32_t *data)
{
    uint32_t target = data[0];
    uint32_t ack = data[1];
    printf("%s: not implemented yet: target(%d), ack(%d)\r\n", 
		__func__, target, ack);
    return 0;
}

static int pm_set_configuration(uint32_t sender, uint32_t *data)
{
    uint32_t node_id = data[0];
    printf("%s: not implemented yet: node_id(%d)\r\n", 
		__func__, node_id);
    return 0;
}

static int pm_get_node_status(uint32_t sender, uint32_t *data, uint32_t *reply)
{
            /* read node status 
 * 		[0] - Current power state of the node
 * 		[1] - Current requirements for the node (slave nodes only)
 * 		[2] - Current usage status for the node (slave nodes only)
            */
    uint32_t node_id = data[0];
    printf("%s: not implemented yet: node_id(%d)\r\n", node_id);
    return 3;
}

static int pm_get_op_characteristic(uint32_t sender, uint32_t *data, uint32_t * reply)
{
    uint32_t node_id = data[0];
    uint32_t type = data[1];
    printf("%s: not implemented yet: node_id(%d), type(%d)\r\n", 
		__func__, node_id, type);
    return 1;
}

static int pm_register_notifier(uint32_t sender, uint32_t *data)
{
    uint32_t node_id = data[0];
    uint32_t event = data[1];
    uint32_t wake = data[2];
    uint32_t enable = data[3];
    printf("%s: not implemented yet: node_id(%d), event(0x%x), wake(0x%x), enable(%d)\r\n", 
		__func__, node_id, event, wake, enable);
    return 0;
}

static int pm_abort_suspend(uint32_t sender, uint32_t *data)
{
    uint32_t reason = data[0];
    uint32_t target = data[1];
    printf("%s: not implemented yet: reason(%d), target(%d)\r\n", 
		__func__, reason, target);
    return 0;
}

static int pm_req_wakeup(uint32_t sender, uint32_t * data)
{
    uint32_t target = data[0];
    uint32_t addr_low = data[1];
    uint32_t addr_high = data[2];
    uint32_t ack = data[3];
    printf("%s: not implemented yet: target(%d), addr_low(0x%x), addr_high(0x%x), ack(%d)\r\n", 
		__func__, target, addr_low, addr_high, ack);
    return 0;
}

static int pm_set_wakeup_source(uint32_t sender, uint32_t *data)
{
    uint32_t target = data[0];
    uint32_t wakeup_node = data[1];
    uint32_t enable = data[2];
    printf("%s: not implemented yet: target(%d), wakeup_node(0x%x), enable(%d)\r\n", 
		__func__, target, wakeup_node, enable);
    return 0;
}

static int pm_system_shutdown(uint32_t sender, uint32_t *data)
{
    uint32_t type = data[0];
    uint32_t sub_type = data[1];
    printf("%s: not implemented yet: type(%d), subtype(%d)\r\n", 
		__func__, type, sub_type);
    return 0;
}

static int pm_req_node(uint32_t sender, uint32_t *data)
{
    uint32_t node_id = data[0];
    uint32_t capabilities = data[1];
    uint32_t qos = data[2];
    uint32_t ack = data[3];
    printf("%s: not implemented yet: node_id(%d), capabilities(0x%x), qos(0x%x), ack(%d)\r\n", 
		__func__, node_id, capabilities, qos, ack);
    return 0;
}

static int pm_release_node(uint32_t sender, uint32_t *data)
{
    uint32_t target = data[0];
    printf("%s: not implemented yet: target(%d)\r\n", 
		__func__, target);
    return 0;
}

static int pm_set_requirement(uint32_t sender, uint32_t *data)
{
    uint32_t node_id = data[0];
    uint32_t capabilities = data[1];
    uint32_t qos = data[2];
    uint32_t ack = data[3];
    printf("%s: not implemented yet: node_id(%d), capabilities(0x%x), qos(0x%x), ack(%d)\r\n", 
		__func__, node_id, capabilities, qos, ack);
    return 0;
}

static int pm_set_max_latency(uint32_t sender, uint32_t *data)
{
    uint32_t target = data[0];
    uint32_t latency = data[1];
    printf("%s: not implemented yet: target(%d), latency(%d)\r\n", 
		__func__, target, latency);
    return 0;
}

static int pm_reset_assert(uint32_t sender, uint32_t *data)
{
    uint32_t reset_id = data[0];
    uint32_t assert = data[1];
    printf("%s: not implemented yet: reset_id(%d), assert(%d)\r\n", 
		__func__, reset_id, assert);
    return 0;
}

static int pm_reset_get_status(uint32_t sender, uint32_t *data, uint32_t * reply)
{
    uint32_t reset_id = data[0];
    printf("%s: not implemented yet: reset_id(%d)\r\n", 
		__func__, reset_id);
    return 1;
}

static int pm_mmio_write(uint32_t sender, uint32_t *data)
{
    uint32_t address = data[0];
    uint32_t mask = data[1];
    uint32_t value = data[2];
    printf("%s: not implemented yet: address(0x%x), mask(0x%x), value(0x%x)\r\n", 
		__func__, address, mask, value);
    return 0;
}

static int pm_mmio_read(uint32_t sender, uint32_t *data, uint32_t *reply)
{
    uint32_t address = data[0];
    printf("%s: not implemented yet: address(0x%x)\r\n", 
		__func__, address);
    return 1;
}

static int pm_get_chipid(uint32_t sender, uint32_t *data, uint32_t *reply)
{
    printf("%s: not implemented yet: )\r\n", __func__);
    return 2;
}

static int pm_init_finalize(uint32_t sender, uint32_t *data)
{
    printf("%s: not implemented yet: )\r\n", __func__);
    return 0;
}

static int not_implemented(char * fname, int return_size)
{
    printf("%s: not implemented yet: )\r\n", fname);
    return return_size;
}


#define PSCI_ARG_OFFSET 2
/* handle_psci()
   packet format:
      cmd->msg[0]: CMD_PSCI
      cmd->msg[1]: sender node id
      cmd->msg[2]: PSCI command
      cmd->msg[3..]: argument of the PSCI command
*/
int handle_psci(struct cmd * cmd, void *reply_buf)
{
    uint32_t *data, *arg_data;
    uint8_t * reply_u8 = (uint8_t *)reply_buf;
    uint32_t * reply;
    uint32_t sender;
 
    data = (uint32_t *) &(cmd->msg[CMD_MSG_PAYLOAD_OFFSET]);
    reply = (uint32_t *) reply_u8;
    /* TODO: do the work and return the right results */
    int i;
    for (i = 0; i < 7; ++i) {
        printf("0x%x ", data[i]);
    }
    printf("\r\n");
    sender = data[0];
    arg_data = &data[PSCI_ARG_OFFSET];
    switch (data[1]) {	
	case PM_GET_API_VERSION: /* impl */
            reply[0] = PM_VERSION;
            for (i = 0; i < 7; ++i)
                printf("0x%x ", reply_u8[i]);
            printf("\r\n");
	    printf("%s: PM_VERSION = 0x%x\r\n", __func__, reply[0]);
            return 1;
        case PM_SET_CONFIGURATION:
            return pm_set_configuration(sender, arg_data);
        case PM_GET_NODE_STATUS:
            return pm_get_node_status(sender, arg_data, reply);
        case PM_GET_OP_CHARACTERISTIC:
            return pm_get_op_characteristic(sender, arg_data, reply);
        case PM_REGISTER_NOTIFIER:
            return pm_register_notifier(sender, arg_data);
        case PM_REQ_SUSPEND:
            return pm_req_suspend(sender, arg_data);
        case PM_SELF_SUSPEND:
            return pm_self_suspend(sender, arg_data);
        case PM_FORCE_POWERDOWN:
            return pm_force_powerdown(sender, arg_data);
        case PM_ABORT_SUSPEND:
            return pm_abort_suspend(sender, arg_data);
        case PM_REQ_WAKEUP:
            return pm_req_wakeup(sender, arg_data);
        case PM_SET_WAKEUP_SOURCE:
            return pm_set_wakeup_source(sender, arg_data);
        case PM_SYSTEM_SHUTDOWN:
            return pm_system_shutdown(sender, arg_data);
        case PM_REQ_NODE:
            return pm_req_node(sender, arg_data);
        case PM_RELEASE_NODE:
            return pm_release_node(sender, arg_data);
        case PM_SET_REQUIREMENT:
            return pm_set_requirement(sender, arg_data);
        case PM_SET_MAX_LATENCY:
            return pm_set_max_latency(sender, arg_data);
        case PM_RESET_ASSERT:
            return pm_reset_assert(sender, arg_data);
        case PM_RESET_GET_STATUS:
            return pm_reset_get_status(sender, arg_data, reply);
        case PM_MMIO_WRITE:
            return pm_mmio_write(sender, arg_data);
        case PM_MMIO_READ:
            return pm_mmio_read(sender, arg_data, reply);
        case PM_INIT_FINALIZE:
            return pm_init_finalize(sender, arg_data);
            break;
        case PM_GET_CHIPID:
            return pm_get_chipid(sender, arg_data, reply);
        case PM_SECURE_RSA_AES:
            return not_implemented("PM_SECURE_RSA_AES", 0);
        case PM_SECURE_SHA:
            return not_implemented("PM_SECURE_RSA_AES", 0);
        case PM_SECURE_RSA:
            return not_implemented("PM_SECURE_RSA_AES", 0);
        case PM_SECURE_IMAGE:
            return not_implemented("PM_SECURE_RSA_AES", 2);
	default:
            return 0;
    }
    return 0;
}
