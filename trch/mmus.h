#ifndef MMUS_H
#define MMUS_H

#include <stdint.h>

int rt_mmu_init();
int rt_mmu_deinit();
int rt_mmu_map(uint64_t vaddr, uint64_t paddr, unsigned sz);
int rt_mmu_unmap(uint64_t vaddr, unsigned sz);

#endif // MMUS_H
