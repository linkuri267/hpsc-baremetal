#include <stdint.h>

#include "printf.h"

#include "intc.h"

#define NVIC_ISER0_OFFSET 0x100

struct nvic {
        volatile uint32_t *base;
};

static struct nvic nvic = {0}; // support only one to make the interface simpler

void intc_init(volatile uint32_t *base)
{
    nvic.base = base;
}

void intc_int_enable(unsigned irq, irq_type_t type)
{
    volatile uint32_t *intc_reg_ie =
        (volatile uint32_t *)((volatile uint8_t *)nvic.base +
                NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <- 0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie |= intc_reg_ie_val;
}

void intc_int_disable(unsigned irq)
{
    // Enable interrupt from mailbox
    volatile uint32_t *intc_reg_ie =
        (volatile uint32_t *)((volatile uint8_t *)nvic.base +
                NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <&- ~0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie &= ~intc_reg_ie_val;
}
