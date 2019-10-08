#ifndef SMC_H
#define SMC_H

#include <stdbool.h>
#include <stdint.h>

#define SMC_INTERFACES 2
#define SMC_IFACE_MASK_ALL 0x3

#define SMC_CHIPS_PER_INTERFACE 4
#define SMC_CHIP_MASK_ALL 0xf

enum smc_iface_type {
    SMC_IFACE_SRAM = 0,
    SMC_IFACE_NAND,
};

#define SMC_IFACE_SRAM_MASK (1 << SMC_IFACE_SRAM)
#define SMC_IFACE_NAND_MASK (1 << SMC_IFACE_NAND)

/* mem_cfg register: memory type values */
enum smc_mem_type {
    SMC_MEM_NONE = 0x0,
    SMC_MEM_SRAM_NON_MULTIPLEXED = 0x1,
    SMC_MEM_SRAM_MULTIPLEXED = 0x3,
    SMC_MEM_NAND = 0x2,
};

struct smc_mem_chip_cfg {
    unsigned width;
    bool sync;
    bool adv;
    unsigned t_rc;
    unsigned t_wc;
    unsigned t_ceoe;
    unsigned t_wp;
    unsigned t_pc;
    unsigned t_tr;
    uint32_t addr_mask[SMC_CHIPS_PER_INTERFACE];
    uint32_t addr_match[SMC_CHIPS_PER_INTERFACE];
};

struct smc_mem_iface_cfg {
    unsigned ext_addr_bits;
    bool cre;
    unsigned chips;
    const struct smc_mem_chip_cfg *chip_cfgs[SMC_CHIPS_PER_INTERFACE];
};

struct smc_mem_cfg {
    struct smc_mem_iface_cfg iface[SMC_INTERFACES];
};


struct smc;

struct smc *smc_init(uintptr_t base, const struct smc_mem_cfg *cfg,
                     uint8_t iface_mask, uint8_t chip_mask);
void smc_deinit(struct smc *);
uint8_t *smc_get_base_addr(struct smc *s, enum smc_iface_type iface,
                           unsigned rank);
#endif // SMC_H
