#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H

typedef enum { // bitmask
    COMPONENT_INVALID = 0,
    COMPONENT_RTPS    = 0x1,
    COMPONENT_HPPS    = 0x2,
    NUM_COMPONENTS    = 2
} component_t;

#endif // SUBSYSTEM_H
