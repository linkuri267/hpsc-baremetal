#ifndef MEM_H
#define MEM_H

void bzero(void *p, int size);

volatile void *vmem_set(volatile void *s, int c, unsigned n);

volatile void *vmem_cpy(volatile void *dest, void *src, unsigned n);

void *mem_vcpy(void *dest, volatile void *src, unsigned n);

#endif // MEM_H
