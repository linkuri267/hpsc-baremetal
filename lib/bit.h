#ifndef BIT_H
#define BIT_H

#include <stdint.h>
#include <stdbool.h>

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (typeof(x))(((uint32_t)(x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))
#define ALIGNED(x, bits) (x == ALIGN(x, bits))

static inline uint32_t byte_swap32(uint32_t x)
{
    return ((x & 0x000000ffU) << 24) |
           ((x & 0x0000ff00U) <<  8) |
           ((x & 0x00ff0000U) >>  8) |
           ((x & 0xff000000U) >> 24);
}

static inline uint64_t byte_swap64(uint64_t x)
{
    return  ((x & 0x00000000000000ffULL) << (8 * 7)) |
            ((x & 0x000000000000ff00ULL) << (8 * 5)) |
            ((x & 0x0000000000ff0000ULL) << (8 * 3)) |
            ((x & 0x00000000ff000000ULL) << (8 * 1)) |
            ((x & 0x000000ff00000000ULL) >> (8 * 1)) |
            ((x & 0x0000ff0000000000ULL) >> (8 * 3)) |
            ((x & 0x00ff000000000000ULL) >> (8 * 5)) |
            ((x & 0xff00000000000000ULL) >> (8 * 7));
}

unsigned log2_of_pow2(unsigned long v);
bool is_pow2(unsigned v);

#endif // BIT_H
