#ifndef NVIC_H
#define NVIC_H

#include <stdint.h>

void nvic_init(uintptr_t scs_base);
unsigned nvic_num_ints();

void nvic_int_enable(unsigned irq);
void nvic_int_disable(unsigned irq);

void nvic_disable_all();


// For use by intc.h common adapter

struct irq;

struct irq *nvic_request(unsigned irqn);
void nvic_release(struct irq *irq);

#endif // NVIC_H
