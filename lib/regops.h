#ifndef REGOPS_H
#define REGOPS_H

#include <stdint.h>
#include "printf.h"

#define REG_WRITE32(reg, val) reg_write32(#reg, reg, val)
#define REG_WRITE64(reg, val) reg_write64(#reg, reg, val)

#define REG_SET32(reg, val) reg_set32(#reg, reg, val)
#define REG_CLEAR32(reg, val) reg_clear32(#reg, reg, val)

#define REG_READ32(reg) reg_read32(#reg, reg)
#define REG_READ64(reg) reg_read64(#reg, reg)

static inline void reg_write32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    printf("%32s: %p <- %x\r\n", name, addr, val);
    *addr = val;
}
static inline void reg_write64(const char *name, volatile uint64_t *addr, uint64_t val)
{
    printf("%32s: %p <- %08x%08x\r\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    *addr = val;
}
static inline uint32_t reg_read32(const char *name, volatile uint32_t *addr)
{
    uint32_t val = *addr;
    printf("%32s: %p -> %x\r\n", name, addr, val);
    return val;
}
static inline uint64_t reg_read64(const char *name, volatile uint64_t *addr)
{
    uint64_t val = *addr;
    printf("%32s: %p -> %08x%08x\r\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    return val;
}
static inline void reg_set32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    printf("%32s: %p |= %x\r\n", name, addr, val);
    *addr |= val;
}
static inline void reg_clear32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    printf("%32s: %p &= ~%x\r\n", name, addr, val);
    *addr &= ~val;
}

#endif // REGOPS_H
