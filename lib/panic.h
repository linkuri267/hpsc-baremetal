#ifndef PANIC_H
#define PANIC_H

#include "printf.h"

#define ASSERT(cond) \
        if (!(cond)) { \
                printf("ASSERT: %s:%u: " #cond "\r\n", __FILE__, __LINE__); \
                panic("ASSERT"); \
        }

void panic(const char *msg);

#endif // PANIC_H
