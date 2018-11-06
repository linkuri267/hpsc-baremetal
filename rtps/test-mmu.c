#include <stdint.h>

#include "printf.h"
#include "test.h"

int test_rt_mmu()
{
    // Translated by MMU via identity map (in HPPS LOW DRAM)
    volatile uint32_t *addr = (volatile uint32_t *)0x8e100000;
    uint32_t valref = 0xf00dcafe;
    printf("%p <- %08x\r\n", addr, valref);
    *addr = valref;
    uint32_t val = *addr;
    if (val != valref) {
        printf("mmu test: value at %p differs from written value: %x != %x\r\n",
               addr, val, valref);
        return 1;
    }
    printf("%p -> %08x\r\n", addr, val);

    // Translated by MMU (test configured to HPPS HIGH DRAM, 0x100000000)
    addr = (volatile uint32_t *)0xc0000000;

    valref = 0xbeeff00d; // written test code on TRCH
    val = *addr;
    printf("%p -> %08x\r\n", addr, val);
    if (val != valref) {
        printf("mmu test: value at %p differs from expected value: %x != %x\r\n",
               addr, val, valref);
        return 1;
    }

    valref = 0xdeadbeef;
    printf("%p <- %08x\r\n", addr, valref);
    *addr = valref;
    val = *addr;
    if (val != valref) {
        printf("mmu test: value at %p differs from written value: %x != %x\r\n",
               addr, val, valref);
        return 1;
    }
    printf("%p -> %08x\r\n", addr, val);

    return 0;
}
