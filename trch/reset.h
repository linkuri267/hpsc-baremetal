#ifndef RESET_H
#define RESET_H

#include <stdbool.h>
#include "subsys.h"

int reset_assert(subsys_t subsys);
int reset_release(subsys_t subsys);

#endif // RESET_H
