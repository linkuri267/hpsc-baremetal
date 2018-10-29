#ifndef MMU_H
#define MMU_H

// PT addr has to be 32-bit since TRCH must be able to access it
int mmu_init(uint32_t pt_addr);
int mmu_map(uint64_t vaddr, uint64_t paddr, unsigned sz);
int mmu_unmap(uint64_t vaddr, uint64_t paddr, unsigned sz);

#endif // MMU_H
