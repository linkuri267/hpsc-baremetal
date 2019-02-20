#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "wdt.h"

int watchdog_init(struct wdt **wdt_ptr);
void watchdog_deinit();
void watchdog_kick();

#endif // WATCHDOG_H
