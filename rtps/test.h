#ifndef TEST_H
#define TEST_H

#include "dma.h"
#include "rti-timer.h"

// Needs to be exposed since referenced from the GIC ISR
// TODO: add cb registration to irq framework to avoid this
extern struct dma *rtps_dma;

int test_float();
int test_sort();
int test_gtimer();
int test_rt_mmu();
int test_rtps_mmu();
int test_rtps_trch_mailbox();
int test_rtps_dma();
int test_wdt();
int test_core_rti_timer(struct rti_timer **tmr_ptr);
int test_r52_smp();

#endif // TEST_H
