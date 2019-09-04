#include "bit.h"

#include "panic.h"

unsigned log2_of_pow2(unsigned long v)
{
    ASSERT(v);
    int b = 0;
    while ((v & 0x1) == 0x0) {
        v >>= 1;
        b++;
    }
    ASSERT(v == 0x1); // power of 2
    return b;
}
