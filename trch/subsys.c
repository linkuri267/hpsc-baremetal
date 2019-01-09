#include "panic.h"
#include "str.h"
#include "subsys.h"

static const struct cpu_group cpu_groups[] = {
    [CPU_GROUP_TRCH]       = { SUBSYS_TRCH,     COMP_CPUS_TRCH,     TRCH_NUM_CORES },
    [CPU_GROUP_RTPS_R52]   = { SUBSYS_RTPS_R52, COMP_CPUS_RTPS_R52, RTPS_R52_NUM_CORES },
    [CPU_GROUP_RTPS_R52_0] = { SUBSYS_RTPS_R52, COMP_CPU_RTPS_R52_0, 1 },
    [CPU_GROUP_RTPS_R52_1] = { SUBSYS_RTPS_R52, COMP_CPU_RTPS_R52_1, 1 },
    [CPU_GROUP_RTPS_A53]   = { SUBSYS_RTPS_A53, COMP_CPUS_RTPS_A53, RTPS_A53_NUM_CORES },
    [CPU_GROUP_HPPS]       = { SUBSYS_HPPS,     COMP_CPUS_HPPS,     HPPS_NUM_CORES },
};

static char subsys_name_buf[16];

static const char *subsys_names[] = {
    "TRCH",
    "RTPS_R52",
    "RTPS_A53",
    "HPPS",
};

const struct cpu_group *subsys_cpu_group(enum cpu_group_id id)
{
    ASSERT(id < NUM_CPU_GROUPS);
    return &cpu_groups[id];
}

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
