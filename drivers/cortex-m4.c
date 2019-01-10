#include "systick.h"

#include "arm.h" // the interface being implemented

void sys_ints_enable()
{
    systick_enable();
    // There are others, but not implemented yet because not used
}

void sys_ints_disable()
{
    systick_disable();
    // others (see comment above)
}
