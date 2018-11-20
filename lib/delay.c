#include "delay.h"

// A very very approximate conversion factor.
// You'd think it would be less than 1, but no (empirically determined).
#define CYCLES_TO_ITERS 25

void delay(unsigned cycles)
{
    volatile unsigned work = 0;
    for (int i = 0; i < cycles * CYCLES_TO_ITERS; ++i)
        work++;
}
