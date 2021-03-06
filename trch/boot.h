#ifndef BOOT_H
#define BOOT_H

#include <stdbool.h>

#include "subsys.h"
#include "syscfg.h"

struct memfs;

void boot_request(subsys_t subsys);
bool boot_pending();
int boot_handle(subsys_t *subsys);
int boot_reboot(subsys_t subsys, struct syscfg *cfg, struct memfs *fs);

#endif // BOOT_H
