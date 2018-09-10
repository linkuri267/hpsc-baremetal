#ifndef RESET_H
#define RESET_H

#include <stdbool.h>

typedef enum {
    COMPONENT_RTPS = 1,
    COMPONENT_HPPS,
} component_t;

void reset_component(component_t component, bool first_boot);

#endif // RESET_H
