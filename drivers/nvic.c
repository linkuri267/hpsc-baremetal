#include <stdint.h>

#include "printf.h"
#include "object.h"
#include "intc.h"

#include "nvic.h"

#define NVIC_ISER0_OFFSET 0x100

#define MAX_IRQS 128

struct irq {
    struct object obj;
    unsigned n;
};

struct nvic {
    volatile uint32_t *base;
    struct irq irqs[MAX_IRQS];
};

static struct nvic nvic = {0}; // support only one to make the interface simpler

void nvic_int_enable(unsigned irq)
{
    volatile uint32_t *intc_reg_ie =
        (volatile uint32_t *)((volatile uint8_t *)nvic.base +
                NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <- 0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie |= intc_reg_ie_val;
}

void nvic_int_disable(unsigned irq)
{
    // Enable interrupt from mailbox
    volatile uint32_t *intc_reg_ie =
        (volatile uint32_t *)((volatile uint8_t *)nvic.base +
                NVIC_ISER0_OFFSET + (irq/32)*4);
    uint32_t intc_reg_ie_val = 1 << (irq % 32);

    printf("NVIC IE IRQ #%u: %p <&- ~0x%08lx\r\n", irq, intc_reg_ie, intc_reg_ie_val);
    *intc_reg_ie &= ~intc_reg_ie_val;
}

struct irq *nvic_request(unsigned irqn)
{
    struct irq *irq = OBJECT_ALLOC(nvic.irqs);
    irq->n = irqn;
    return irq;
}

void nvic_release(struct irq *irq)
{
    OBJECT_FREE(irq);
}

static void nvic_op_int_enable(struct irq *irq)
{
    nvic_int_enable(irq->n);
}

static void nvic_op_int_disable(struct irq *irq)
{
    nvic_int_disable(irq->n);
}
unsigned nvic_op_int_num(struct irq *irq)
{
    return irq->n;
}
unsigned nvic_op_int_type(struct irq *irq)
{
    return 0; // no types for NVIC
}

static const struct intc_ops nvic_ops = {
    .int_enable = nvic_op_int_enable,
    .int_disable = nvic_op_int_disable,
    .int_num = nvic_op_int_num,
    .int_type = nvic_op_int_type,
};

void nvic_init(volatile uint32_t *base)
{
    nvic.base = base;
    intc_register(&nvic_ops);
}
