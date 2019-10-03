#ifndef SMC_H
#define SMC_H

#include <stdbool.h>
#include <stdint.h>

#define SMC_INTERFACES 2
#define SMC_CHIPS_PER_INTERFACE 4

struct smc_mem_iface_cfg {
    unsigned chips;
    unsigned width;
    unsigned ext_addr_bits;
    bool sync;
    bool adv;
    bool cre;
    unsigned t_rc;
    unsigned t_wc;
    unsigned t_ceoe;
    unsigned t_wp;
    unsigned t_pc;
    unsigned t_tr;
    uint32_t addr_mask[SMC_CHIPS_PER_INTERFACE];
    uint32_t addr_match[SMC_CHIPS_PER_INTERFACE];
};

struct smc_mem_cfg {
    struct smc_mem_iface_cfg iface[SMC_INTERFACES];
};


struct smc;

struct smc *smc_init(uintptr_t base, struct smc_mem_cfg *cfg);
void smc_deinit(struct smc *);
uint32_t *smc_get_base_addr(uintptr_t base, int interface, int rank);
void smc_boot_init(uintptr_t base, int mem_rank, struct smc_mem_iface_cfg *iface_cfg, bool failover);

#endif // SMC_H
