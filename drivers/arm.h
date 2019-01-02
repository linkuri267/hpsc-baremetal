#ifndef ARM_H
#define ARM_H

// Portable across ARMv7 and ARMv8 Aarch32

static inline void int_enable()
{
    asm volatile ("cpsie i");
}
static inline void int_disable()
{
    asm volatile ("cpsid i");
}

#endif // ARM_H
