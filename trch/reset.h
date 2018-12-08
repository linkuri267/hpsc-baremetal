#ifndef RESET_H
#define RESET_H

#include <stdbool.h>

typedef enum {
    COMPONENT_INVALID = 0,
    COMPONENT_RTPS,
    COMPONENT_HPPS,
} component_t;

int reset_component(component_t component);

#endif // RESET_H
