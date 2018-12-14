#include <stdint.h>

#include "printf.h"

void panic(const char *msg)
{
    printf("PANIC HALT: %s\r\n", msg);
    while (1); // halt
}

void dump_buf(const char *name, uint32_t *buf, unsigned words)
{
    printf("%s:\r\n", name);
    for (unsigned i = 0; i < words; ++i) {
        if (i % 8 == 0)
            printf("0x%p: ", buf + i);
        printf("%x ", buf[i]);
	if ((i + 1) % 8 == 0)
	    printf("\r\n");
    }
    printf("\r\n");
}
