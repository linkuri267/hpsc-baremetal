#ifndef BOOT_H
#define BOOT_H

#include <stdbool.h>

#include "subsys.h"

int boot_config();
void boot_request(subsys_t subsys);
bool boot_pending();
int boot_handle(subsys_t *subsys);
int boot_reboot(subsys_t subsys);

#endif // BOOT_H
