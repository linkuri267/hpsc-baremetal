#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "wdt.h"

extern struct wdt *wdt; // must be in global scope since needed by global ISR

int watchdog_init();
void watchdog_deinit();
void watchdog_kick();

#endif // WATCHDOG_H
