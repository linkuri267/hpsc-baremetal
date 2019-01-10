#include "panic.h"

#include "intc.h"

static const struct intc_ops *intc_ops = NULL;

void intc_register(const struct intc_ops *ops)
{
    intc_ops = ops;
}

void intc_int_enable(struct irq *irq)
{
    ASSERT(intc_ops && intc_ops->int_enable);
    intc_ops->int_enable(irq);
}

void intc_int_disable(struct irq *irq)
{
    ASSERT(intc_ops && intc_ops->int_disable);
    intc_ops->int_disable(irq);
}

void intc_disable_all()
{
    ASSERT(intc_ops && intc_ops->disable_all);
    intc_ops->disable_all();
}

unsigned intc_int_num(struct irq *irq)
{
    ASSERT(intc_ops && intc_ops->int_num);
    return intc_ops->int_num(irq);
}
unsigned intc_int_type(struct irq *irq)
{
    ASSERT(intc_ops && intc_ops->int_type);
    return intc_ops->int_type(irq);
}
