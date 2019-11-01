#include <stdint.h>

#include "console.h"
#include "arm.h"
#include "intc.h"

void panic(const char *msg)
{
    int_disable(); // raises priority, so ISRs won't run but WFI still returns

    // To stay in WFI, we need to disable internal and external interrupts
    sys_ints_disable(); // internal interrupts that bypass int controller
    intc_disable_all(); // external interrupts

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
