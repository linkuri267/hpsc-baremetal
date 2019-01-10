#ifndef INTC_H
#define INTC_H

#include <stdint.h>

// This interface lets consumers reach the system's interrupt controller
// driver, without knowing which driver is in use.  This is useful for drivers,
// whereas from system-specific code, it is simpler to just use the direct API
// of the respective interrupt controller.

struct irq;

struct intc_ops {
    void (*int_enable)(struct irq *irq);
    void (*int_disable)(struct irq *irq);
    void (*disable_all)(void);

    // For debugging info purpose
    unsigned (*int_num)(struct irq *irq);
    unsigned (*int_type)(struct irq *irq);
};

void intc_register(const struct intc_ops *ops);

void intc_int_enable(struct irq *irq);
void intc_int_disable(struct irq *irq);
void intc_disable_all();

// For debugging info purposes, since the object is opaque
unsigned intc_int_num(struct irq *irq);
unsigned intc_int_type(struct irq *irq);

#endif // INTC_H
