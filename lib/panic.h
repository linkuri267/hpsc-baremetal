#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

#include "printf.h"

#define ASSERT(cond) \
        if (!(cond)) { \
                printf("ASSERT: %s:%u: " #cond "\r\n", __FILE__, __LINE__); \
                panic("ASSERT"); \
        }

void panic(const char *msg);
void dump_buf(const char *name, uint32_t *buf, unsigned words);

#endif // PANIC_H
