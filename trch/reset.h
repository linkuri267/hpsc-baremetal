#ifndef RESET_H
#define RESET_H

#include <stdbool.h>
#include "subsys.h"

int reset_assert(comp_t comps);
int reset_release(comp_t comps);

#endif // RESET_H
