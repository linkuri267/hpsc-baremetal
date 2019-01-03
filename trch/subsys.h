#ifndef SUBSYS_H
#define SUBSYS_H

typedef enum { // bitmask
    SUBSYS_INVALID = 0,
    SUBSYS_RTPS    = 0x1,
    SUBSYS_HPPS    = 0x2,
    NUM_SUBSYSS    = 2
} subsys_t;

typedef enum { // bitmask
#define COMP_CPUS_SHIFT_TRCH 0
    COMP_CPU_TRCH    = 0x0001,

#define COMP_CPUS_SHIFT_RTPS 1
    COMP_CPU_RTPS_0  = 0x0002,
    COMP_CPU_RTPS_1  = 0x0004,

#define COMP_CPUS_SHIFT_HPPS 3
    COMP_CPU_HPPS_0  = 0x0008,
    COMP_CPU_HPPS_1  = 0x0010,
    COMP_CPU_HPPS_2  = 0x0020,
    COMP_CPU_HPPS_3  = 0x0040,
    COMP_CPU_HPPS_4  = 0x0080,
    COMP_CPU_HPPS_5  = 0x0100,
    COMP_CPU_HPPS_6  = 0x0200,
    COMP_CPU_HPPS_7  = 0x0400,
} comp_t;

#define COMP_CPUS_RTPS (COMP_CPU_RTPS_0 | COMP_CPU_RTPS_1)
#define COMP_CPUS_HPPS \
        (COMP_CPU_HPPS_0 | COMP_CPU_HPPS_1 |  COMP_CPU_HPPS_2 | COMP_CPU_HPPS_3 | \
         COMP_CPU_HPPS_4 | COMP_CPU_HPPS_5 |  COMP_CPU_HPPS_6 | COMP_CPU_HPPS_7)

#endif // SUBSYS_H
