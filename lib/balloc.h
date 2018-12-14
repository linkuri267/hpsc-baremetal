#ifndef BALLOC_H
#define BALLOC_H

#include <stdint.h>

#include "bit.h"

struct balloc;

struct balloc *balloc_create(const char *name, void *addr, unsigned size);
void balloc_destroy(struct balloc *ba);

void *balloc_alloc(struct balloc *ba, unsigned sz, unsigned align_bits);
int balloc_free(struct balloc *ba, void *addr, unsigned sz);

#endif // BALLOC_H
