#include <stdint.h>

#include "printf.h"
#include "object.h"
#include "regops.h"
#include "intc.h"

#include "nvic.h"

#define NVIC_ICTR  0x004
#define NVIC_ISER0 0x100
#define NVIC_ICER0 0x180
#define NVIC_ICPR0 0x280

#define NVIC_ICTR__INTLINESNUM__MASK 0xf

#define MAX_IRQS 240

struct irq {
    struct object obj;
    unsigned n;
};

struct nvic {
    uintptr_t base;
    struct irq irqs[MAX_IRQS];
};

static struct nvic nvic = {0}; // support only one to make the interface simpler

void nvic_int_enable(unsigned irq)
{
    printf("NVIC IRQ #%u: enable\r\n", irq);
    REGB_WRITE32(nvic.base, NVIC_ISER0 + (irq / 32) * 4, 1 << (irq % 32));
}

void nvic_int_disable(unsigned irq)
{
    printf("NVIC IRQ #%u: disable\r\n", irq);
    REGB_WRITE32(nvic.base, NVIC_ICER0 + (irq / 32) * 4, 1 << (irq % 32));
}

unsigned nvic_num_ints()
{
    return (REGB_READ32(nvic.base, NVIC_ICTR) & NVIC_ICTR__INTLINESNUM__MASK) * 32;
}

void nvic_disable_all()
{
    int regs = nvic_num_ints() / 32;
    printf("NVIC: disable all: regs %u\r\n", regs);
    for (unsigned i = 0; i < regs; ++i)
        REGB_WRITE32(nvic.base, NVIC_ICER0 + i * 4, 0xffffffff);
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
static void nvic_op_disable_all()
{
    nvic_disable_all();
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
    .disable_all = nvic_op_disable_all,
    .int_num = nvic_op_int_num,
    .int_type = nvic_op_int_type,
};

void nvic_init(uintptr_t scs_base)
{
    nvic.base = scs_base;
    intc_register(&nvic_ops);
}
