#ifndef GIC_H
#define GIC_H

#include <stdint.h>

#define GIC_INTERNAL   32
#define GIC_NR_SGIS    16

typedef enum {
    GIC_IRQ_TYPE_SPI,
    GIC_IRQ_TYPE_LPI,
    GIC_IRQ_TYPE_PPI,
    GIC_IRQ_TYPE_SGI,
} gic_irq_type_t;

typedef enum {
    GIC_IRQ_CFG_LEVEL,
    GIC_IRQ_CFG_EDGE
} gic_irq_cfg_t;

void gic_init(volatile uint32_t *base);

void gic_int_enable(unsigned irq, gic_irq_type_t type, gic_irq_cfg_t cfg);
void gic_int_disable(unsigned irq, gic_irq_type_t type);

// For use by intc.h common adapter

struct irq;

struct irq *gic_request(unsigned irqn, gic_irq_type_t type, gic_irq_cfg_t cfg);
void gic_release(struct irq *irq);

void gic_intc_int_enable(struct irq *irq);
void gic_intc_int_disable(struct irq *irq);

#endif // GIC_H
