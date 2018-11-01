#ifndef BALLOC_H
#define BALLOC_H

#include <stdint.h>

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (typeof(x))(((uint32_t)(x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))
#define ALIGNED(x, bits) (x == ALIGN(x, bits))

struct balloc;

struct balloc *balloc_create(const char *name, void *addr, unsigned size);
void balloc_destroy(struct balloc *ba);

void *balloc_alloc(struct balloc *ba, unsigned sz, unsigned align_bits);
int balloc_free(struct balloc *ba, void *addr, unsigned sz);

#endif // BALLOC_H
