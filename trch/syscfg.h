#ifndef SYSCFG_H
#define SYSCFG_H

#include "subsys.h"

// Fields in the 32-bit value specifying the boot config
// Exposing since it's an external-facing interface
#define SYSCFG__BIN_LOC__SHIFT             0
#define SYSCFG__BIN_LOC__MASK              (0x7 << SYSCFG__BIN_LOC__SHIFT)
#define SYSCFG__RTPS_MODE__SHIFT           3
#define SYSCFG__RTPS_MODE__MASK            (0x3 << SYSCFG__RTPS_MODE__SHIFT)
#define SYSCFG__SUBSYS__SHIFT              5
#define SYSCFG__SUBSYS__MASK               (0xf << SYSCFG__SUBSYS__SHIFT)
#define SYSCFG__HPPS_ROOTFS_LOC__SHIFT     9
#define SYSCFG__HPPS_ROOTFS_LOC__MASK      (0x7 << SYSCFG__HPPS_ROOTFS_LOC__SHIFT)

enum memdev {
    MEMDEV_TRCH_SMC_SRAM = 0x0,
    MEMDEV_TRCH_SMC_NAND = 0x1,
    MEMDEV_HPPS_SMC_SRAM = 0x2,
    MEMDEV_HPPS_SMC_NAND = 0x3,
    MEMDEV_HPPS_DRAM     = 0x4,
    MEMDEV_RTPS_DRAM     = 0x5,
    MEMDEV_RTPS_TCM      = 0x6,
    MEMDEV_TRCH_SRAM     = 0x7,
};

struct syscfg {
    enum memdev bin_loc;
    subsys_t subsystems; // bitmask of subsystems to boot
    enum {
        SYSCFG__RTPS_MODE__SPLIT    = 0x0,
        SYSCFG__RTPS_MODE__LOCKSTEP = 0x1,
        SYSCFG__RTPS_MODE__SMP	    = 0x2,
    } rtps_mode;
    enum memdev hpps_rootfs_loc;
};

int syscfg_load(struct syscfg *cfg);
void syscfg_print(struct syscfg *cfg);

const char *memdev_name(enum memdev d);
#endif // SYSCFG_H
