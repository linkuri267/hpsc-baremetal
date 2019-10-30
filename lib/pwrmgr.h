/* Protocol for power management for use by PSCI servers and clients.
 *
 * This protocol is used by the HPSC platform code in ARM Trusted Firmware.
 *
 * Not exactly the same message set as the PSCI protocol, but the messages here
 * map to the PSCI messages closely.
 */
#ifndef LIB_PWRMGR_H
#define LIB_PWRMGR_H

#define PAYLOAD_ARG_CNT		6U
#define PAYLOAD_ARG_SIZE	4U	/* size in bytes */

/* could replace with comp_t from subsys.h */
enum pm_node_id {
	NODE_UNKNOWN = 0,
	NODE_APU,
	NODE_APU_0,
	NODE_APU_1,
	NODE_APU_2,
	NODE_APU_3,
	NODE_APU_4,
	NODE_APU_5,
	NODE_APU_6,
	NODE_APU_7,
	NODE_RPU,
	NODE_RPU_0,
	NODE_RPU_1,
	NODE_RPU_A53,
	NODE_EXTERN,
	NODE_MAX
};

enum pm_api_id {
	/* Miscellaneous API functions: */
	PM_GET_API_VERSION = 1, /* Do not change or move */
	PM_SET_CONFIGURATION,
	PM_GET_NODE_STATUS,
	PM_GET_OP_CHARACTERISTIC,
	PM_REGISTER_NOTIFIER,
	/* API for suspending of PUs: */
	PM_REQ_SUSPEND,
	PM_SELF_SUSPEND,
	PM_FORCE_POWERDOWN,
	PM_ABORT_SUSPEND,
	PM_REQ_WAKEUP,
	PM_SET_WAKEUP_SOURCE,
	PM_SYSTEM_SHUTDOWN,
	PM_API_MAX
};

/**
 * @PM_RET_SUCCESS:		success
 * @PM_RET_ERROR_ARGS:		illegal arguments provided
 * @PM_RET_ERROR_ACCESS:	access rights violation
 * @PM_RET_ERROR_TIMEOUT:	timeout in communication with PMU
 * @PM_RET_ERROR_NOTSUPPORTED:	feature not supported
 * @PM_RET_ERROR_PROC:		node is not a processor node
 * @PM_RET_ERROR_API_ID:	illegal API ID
 * @PM_RET_ERROR_OTHER:		other error
 */
enum pm_ret_status {
	PM_RET_SUCCESS,
	PM_RET_ERROR_ARGS,
	PM_RET_ERROR_ACCESS,
	PM_RET_ERROR_TIMEOUT,
	PM_RET_ERROR_NOTSUPPORTED,
	PM_RET_ERROR_PROC,
	PM_RET_ERROR_API_ID,
	PM_RET_ERROR_FAILURE,
	PM_RET_ERROR_COMMUNIC,
	PM_RET_ERROR_DOUBLEREQ,
	PM_RET_ERROR_OTHER,
};

#endif /* LIB_PWRMGR_H */
