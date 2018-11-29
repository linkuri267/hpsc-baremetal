#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "wdt.h"

// Must be global for the standalone tests for WDTs in test-wdt.c
extern struct wdt *trch_wdt;
extern struct wdt *rtps_wdts[];
extern struct wdt *hpps_wdts[];

int watchdog_trch_start();
void watchdog_trch_stop();
void watchdog_trch_kick();

int watchdog_rtps_start();
void watchdog_rtps_stop();

int watchdog_hpps_start();
void watchdog_hpps_stop();

#endif // WATCHDOG_H
