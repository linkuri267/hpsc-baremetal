#include "panic.h"
#include "str.h"
#include "subsys.h"

static char subsys_name_buf[16];

static const char *subsys_names[] = {
    "RTPS_R52",
    "RTPS_A53",
    "HPPS",
};

const char *subsys_name(subsys_t subsys)
{
    ASSERT(sizeof(subsys_names) / sizeof(subsys_names[0]) == NUM_SUBSYSS);
    subsys_name_buf[0] = '\0';
    int n = 0;
    for (int i = 0; i < NUM_SUBSYSS; ++i) {
        if (subsys & (1 << i)) {
            if (n++ > 0)
                strcat(subsys_name_buf, ",");
            strcat(subsys_name_buf, subsys_names[i]);
        }
    }
    return subsys_name_buf;
}
