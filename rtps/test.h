#ifndef TEST_H
#define TEST_H

#include "dma.h"

// Needs to be exposed since referenced from the GIC ISR
extern struct dma *rtps_dma;

int test_float();
int test_sort();
int test_rt_mmu();
int test_rtps_mmu();
int test_rtps_trch_mailbox();
int test_rtps_dma();

#endif // TEST_H
