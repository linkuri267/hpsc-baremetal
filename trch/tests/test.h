#ifndef TEST_H
#define TEST_H

struct dma;
struct mmu_context;
struct syscfg;

int test_trch_dma();
int test_rt_mmu();
int test_float();
int test_systick();
int test_wdts();
int test_etimer();
int test_core_rti_timer();
int test_shmem();
int test_rio(struct syscfg *syscfg, struct dma *dma);

#endif // TEST_H
