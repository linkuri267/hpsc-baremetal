#ifndef RESET_H
#define RESET_H

#include <stdbool.h>
#include "subsys.h"

enum rtps_r52_mode {
    RTPS_R52_MODE__LOCKSTEP,
    RTPS_R52_MODE__SPLIT,
    RTPS_R52_MODE__SMP,
};

int reset_assert(comp_t comps);
int reset_release(comp_t comps);

int reset_set_rtps_r52_mode(enum rtps_r52_mode m);

#endif // RESET_H
