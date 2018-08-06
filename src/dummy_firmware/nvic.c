#include <stdint.h>

#include <libmspprintf/printf.h>

#include "nvic.h"

#define NVIC_BASE 0xe000e000
#define NVIC_ISER0_OFFSET 0x100

void nvic_int_enable(unsigned irq)
{
    // Enable interrupt from mailbox
    volatile uint32_t *nvic_reg_ie = (volatile uint32_t *)(NVIC_BASE + NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t nvic_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <- 0x%08lx\r\n", irq, nvic_reg_ie, nvic_reg_ie_val);
    *nvic_reg_ie |= nvic_reg_ie_val;
}
