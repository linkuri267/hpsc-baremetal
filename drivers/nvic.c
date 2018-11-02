#include <stdint.h>

#include "printf.h"

#include "intc.h"

#define NVIC_BASE 0xe000e000
#define NVIC_ISER0_OFFSET 0x100

void intc_int_enable(unsigned irq, irq_type_t type)
{
    // Enable interrupt from mailbox
    volatile uint32_t *intc_reg_ie = (volatile uint32_t *)(NVIC_BASE + NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <- 0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie |= intc_reg_ie_val;
}

void intc_int_disable(unsigned irq)
{
    // Enable interrupt from mailbox
    volatile uint32_t *intc_reg_ie = (volatile uint32_t *)(NVIC_BASE + NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <&- ~0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie &= ~intc_reg_ie_val;
}
