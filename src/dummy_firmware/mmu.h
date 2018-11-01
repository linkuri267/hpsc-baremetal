#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#include "balloc.h"

enum mmu_pagesize {
    MMU_PAGESIZE_4KB = 0,
    MMU_PAGESIZE_16KB,
    MMU_PAGESIZE_64KB,
};

struct mmu;
struct mmu_context;
struct mmu_stream;

struct mmu *mmu_create(const char *name, volatile uint32_t *base);
int mmu_destroy(struct mmu *m);

struct mmu_context *mmu_context_create(struct mmu *m, struct balloc *ba, enum mmu_pagesize pgsz);
int mmu_context_destroy(struct mmu_context *ctx);

// Return a status code (zero means success). We don't return a 'mapping'
// object to not burden the user, but we could.
int mmu_map(struct mmu_context *ctx, uint64_t vaddr, uint64_t paddr, unsigned sz);
int mmu_unmap(struct mmu_context *ctx, uint64_t vaddr, unsigned sz);

struct mmu_stream *mmu_stream_create(unsigned master, struct mmu_context *ctx);
int mmu_stream_destroy(struct mmu_stream *s);

void mmu_enable(struct mmu *m);
void mmu_disable(struct mmu *m);

#endif // MMU_H
