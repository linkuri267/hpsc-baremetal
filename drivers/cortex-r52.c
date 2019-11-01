#include "arm.h" // the interface being implemented

void sys_ints_enable()
{
    // None that we care about so far
}

void sys_ints_disable()
{
    // None that we care about so far
}

unsigned self_core_id()
{
    unsigned id;
    asm volatile ("mrc p15, 0, %0, c0, c0, 5":"=r" (id) :);
    return id;
}
