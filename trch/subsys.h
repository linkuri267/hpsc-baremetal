#ifndef SUBSYS_H
#define SUBSYS_H

typedef enum { // bitmask
    SUBSYS_INVALID = 0,
    SUBSYS_RTPS    = 0x1,
    SUBSYS_HPPS    = 0x2,
    NUM_SUBSYSS    = 2
} subsys_t;

#endif // SUBSYS_H
