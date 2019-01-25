#ifndef MEM_H
#define MEM_H

void bzero(void *p, int size);

void *mem_set(void *s, int c, unsigned n);

void *mem_cpy(void *dest, void *src, unsigned n);

#endif // MEM_H
