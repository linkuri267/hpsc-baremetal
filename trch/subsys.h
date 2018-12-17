#ifndef SUBSYS_H
#define SUBSYS_H

typedef enum { // bitmask
    SUBSYS_INVALID = 0,
    SUBSYS_RTPS_SPLIT_0    = 0x1,
    SUBSYS_RTPS_SPLIT_1    = 0x2,
    SUBSYS_RTPS = 0x4,	// lockstep
    SUBSYS_RTPS_SMP        = 0x8,
    SUBSYS_HPPS    = 0x10,
    NUM_SUBSYSS    = 6
} subsys_t;

#endif // SUBSYS_H
