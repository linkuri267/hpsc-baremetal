#ifndef INTC_H
#define INTC_H

#include <stdint.h>

typedef enum {
    IRQ_TYPE_LEVEL = 0,
    IRQ_TYPE_EDGE  = 1
} irq_type_t;

void intc_init(volatile uint32_t *base);
void intc_int_enable(unsigned irq, irq_type_t type);
void intc_int_disable(unsigned irq);

#endif // INTC_H
