#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (typeof(x))(((uint32_t)(x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))
#define ALIGNED(x, bits) (x == ALIGN(x, bits))

int block_init(void *addr, unsigned size);
void *block_alloc(unsigned sz, unsigned align_bits);
int block_free(void *addr, unsigned sz);

#endif // BLOCK_H
