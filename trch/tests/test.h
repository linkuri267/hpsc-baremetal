#ifndef TEST_H
#define TEST_H

struct dma;
struct mmu_context;
struct rio_switch;
struct rio_ep;

int test_trch_dma();
int test_rt_mmu();
int test_float();
int test_systick();
int test_wdts();
int test_etimer();
int test_core_rti_timer();
int test_shmem();
int test_rio_local(struct rio_switch *sw,
                   struct rio_ep *ep0, struct rio_ep *ep1,
                   struct dma *dmac);
int test_rio_offchip_server(struct rio_switch *sw, struct rio_ep *ep);
int test_rio_offchip_client(struct rio_switch *sw, struct rio_ep *ep);

#endif // TEST_H
