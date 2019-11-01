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


// Enables/disables interrupts that bypass the interrupt controller
void sys_ints_enable();
void sys_ints_disable();

unsigned self_core_id();

#endif // ARM_H
