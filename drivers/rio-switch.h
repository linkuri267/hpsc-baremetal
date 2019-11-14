#ifndef RIO_SWITCH_H
#define RIO_SWITCH_H

#include <stdint.h>

enum rio_mapping_type { /* User Guide Table 4 */
    RIO_MAPPING_TYPE_UNICAST   = 0b00,
    RIO_MAPPING_TYPE_MULTICAST = 0b01,
    RIO_MAPPING_TYPE_AGGREGATE = 0b10,
};

struct rio_ep;
struct rio_switch;

struct rio_switch * rio_switch_create(const char *name, bool local,
                                      uintptr_t base);
void rio_switch_destroy(struct rio_switch *s);
void rio_switch_map_local(struct rio_switch *s, rio_devid_t dest,
                          enum rio_mapping_type type, uint8_t port_map);
void rio_switch_unmap_local(struct rio_switch *s, rio_devid_t dest);

int rio_switch_map_remote(struct rio_ep *ep,
                          rio_dest_t switch_dest,
                          rio_devid_t route_dest, enum rio_mapping_type type,
                          uint8_t port_map);
int rio_switch_unmap_remote(struct rio_ep *ep,
                            rio_dest_t switch_dest, rio_devid_t route_dest);

#endif // RIO_SWITCH_H
