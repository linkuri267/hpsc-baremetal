#include <stdint.h>

#include "printf.h"
#include "arm.h"

void panic(const char *msg)
{
    int_disable();
    printf("PANIC HALT: %s\r\n", msg);

    // Halt, but without busylooping. Note: even with masked interrupts, WFI
    // will return on interrupt, but this loop will put us right back to sleep.
    // Furthermore, once an interrupt is latched, WFI will immediately return
    // (at least in Qemu), so despite WFI we end up in a busyloop, oh well.
    while (1)
        asm ("wfi");
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
