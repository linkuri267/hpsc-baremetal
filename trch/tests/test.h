#ifndef TEST_H
#define TEST_H

struct dma;
struct mmu_context;
struct rio_switch;
struct rio_ep;
struct ev_loop;

int test_trch_dma();
int test_rt_mmu();
int test_float();
int test_systick();
int test_wdts();
int test_etimer();
int test_core_rti_timer();
int test_shmem();
int test_rio_start(struct rio_switch *sw,
                   struct rio_ep *ep0, struct rio_ep *ep1,
                   struct dma *dmac, struct ev_loop *ev_loop,
                   bool offline, bool master);

#endif // TEST_H
