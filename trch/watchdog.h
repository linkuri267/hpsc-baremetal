#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "subsys.h"
#include "wdt.h"

// Must be global for the standalone tests for WDTs in test-wdt.c
extern struct wdt *wdts[];

void watchdog_start(comp_t cpus);
void watchdog_stop(comp_t cpus);
void watchdog_kick(comp_t cpus);

void watchdog_start_group(enum cpu_group_id gid);
void watchdog_stop_group(enum cpu_group_id gid);
void watchdog_kick_group(enum cpu_group_id gid);

void watchdog_init_group(enum cpu_group_id gid);
void watchdog_deinit_group(enum cpu_group_id gid);

#endif // WATCHDOG_H
