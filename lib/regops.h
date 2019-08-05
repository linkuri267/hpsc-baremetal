#ifndef REGOPS_H
#define REGOPS_H

#include <stdint.h>
#include "panic.h"

#define MAKE_64BIT_MASK(shift, length) \
    (((~0ULL) >> (64 - (length))) << (shift))

#define REG32(reg, addr)                                              \
    enum { reg = (addr) };                                            \

#define FIELD(reg, field, shift, length)                              \
    enum { reg ## __ ## field ## __SHIFT = (shift)};                  \
    enum { reg ## __ ## field ## __LENGTH = (length)};                \
    enum { reg ## __ ## field ## __MASK = MAKE_64BIT_MASK(shift, length)};

#define FIELD_EX32(storage, reg, field)                              \
    extract32((storage), reg ## __ ## field ## __SHIFT,               \
              reg ## __ ## field ## __LENGTH)

/**
 * extract32:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 32 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 32 bit word. It is valid to request that
 * all 32 bits are returned (ie @length 32 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint32_t extract32(uint32_t value, int start, int length)
{
    ASSERT(start >= 0 && length > 0 && length <= 32 - start);
    return (value >> start) & (~0U >> (32 - length));
}

#define REG_WRITE32(reg, val) reg_write32(#reg, (volatile uint32_t *)(reg), val)
#define REG_WRITE64(reg, val) reg_write64(#reg, (volatile uint64_t *)(reg), val)

#define REGB_WRITE32(base, reg, val) \
        reg_write32(#reg, (volatile uint32_t *)((volatile uint8_t *)(base) + (reg)), val)
#define REGB_WRITE64(base, reg, val) \
        reg_write64(#reg, (volatile uint64_t *)((volatile uint8_t *)(base) + (reg)), val)

#define REG_SET32(reg, val)   reg_set32(#reg,   (volatile uint32_t *)(reg), val)
#define REG_CLEAR32(reg, val) reg_clear32(#reg, (volatile uint32_t *)(reg), val)

#define REGB_SET32(base, reg, val) \
         reg_set32(#reg, (volatile uint32_t *)((volatile uint8_t *)(base) + (reg)), val)
#define REGB_CLEAR32(base, reg, val) \
        reg_clear32(#reg, (volatile uint32_t *)((volatile uint8_t *)(base) + (reg)), val)

#define REG_READ32(reg) reg_read32(#reg, (volatile uint32_t *)(reg))
#define REG_READ64(reg) reg_read64(#reg, (volatile uint32_t *)(reg))

#define REGB_READ32(base, reg) \
        reg_read32(#reg, (volatile uint32_t *)((volatile uint8_t *)(base) + (reg)))
#define REGB_READ64(base, reg) \
        reg_read64(#reg, (volatile uint64_t *)((volatile uint8_t *)(base) + (reg)))

static inline void reg_write32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTF("%32s: %p <- %x\r\n", name, addr, val);
    *addr = val;
}
static inline void reg_write64(const char *name, volatile uint64_t *addr, uint64_t val)
{
    DPRINTF("%32s: %p <- %08x%08x\r\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    *addr = val;
}
static inline uint32_t reg_read32(const char *name, volatile uint32_t *addr)
{
    uint32_t val = *addr;
    DPRINTF("%32s: %p -> %x\r\n", name, addr, val);
    return val;
}
static inline uint64_t reg_read64(const char *name, volatile uint64_t *addr)
{
    uint64_t val = *addr;
    DPRINTF("%32s: %p -> %08x%08x\r\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    return val;
}
static inline void reg_set32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTF("%32s: %p |= %x\r\n", name, addr, val);
    *addr |= val;
}
static inline void reg_clear32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTF("%32s: %p &= ~%x\r\n", name, addr, val);
    *addr &= ~val;
}

#endif // REGOPS_H
