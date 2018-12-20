#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

#include "printf.h"

#define ASSERT(cond) \
        if (!(cond)) { \
                printf("ASSERT: %s:%u: " #cond "\r\n", __FILE__, __LINE__); \
                panic("ASSERT"); \
        }

// Define DEBUG to 1 in the source file that you want to debug
// before the #include statement for this header.
#ifndef DEBUG
#define DEBUG 0
#endif // undef DEBUG

#define DPRINTF(...) \
        if (DEBUG) printf(__VA_ARGS__)

void panic(const char *msg);
void dump_buf(const char *name, uint32_t *buf, unsigned words);

#endif // PANIC_H
