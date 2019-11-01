#include <stdlib.h>

#include "panic.h"
#include "console.h"

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
    // This function is called from panic(...), and ASSERT can call panic, so
    // there's a circular dependency which can run infinitely if we ASSERT.
    // Panic's log message is then buried and the subsystem doesn't halt if
    // the assertion fails.
    // ASSERT(intc_ops && intc_ops->disable_all);
    static int err_once = 0;
    if (intc_ops && intc_ops->disable_all) {
        intc_ops->disable_all();
    } else if (!err_once) {
        err_once = 1;
        printf("INTC: WARN: intc_disable_all: intc_ops not configured\r\n");
    }
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
