#ifndef BOOT_H
#define BOOT_H

#include "subsys.h"

int boot_config();
void boot_request_reboot(subsys_t subsys);
int boot_perform_reboots();

#endif // BOOT_H
