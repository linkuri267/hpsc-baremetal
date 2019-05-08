#ifndef SMC_H
#define SMC_H

#include <stdbool.h>
#include <stdint.h>

#define SMC_INTERFACES 2

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
};

struct smc_mem_cfg {
    struct smc_mem_iface_cfg iface[SMC_INTERFACES];
};


struct smc;

struct smc *smc_init(volatile uint32_t *base, struct smc_mem_cfg *cfg);
void smc_deinit(struct smc *);

#endif // SMC_H
