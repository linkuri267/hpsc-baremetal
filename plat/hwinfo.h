#ifndef HWINFO_H
#define HWINFO_H

// Copies from Qemu Device Tree repository
#include "hpsc-irqs.dtsh"
#include "hpsc-busids.dtsh"

// This file fulfills the role of a device tree

#define HPPS_NUM_CORES 8
#define RTPS_NUM_CORES 2

#define RTPS_GIC_BASE   0x30e00000
#define TRCH_SCS_BASE   0xe000e000

#define RTPS_TRCH_TO_HPPS_SMMU_BASE   ((volatile uint32_t *)0x31100000)
#define RTPS_SMMU_BASE                ((volatile uint32_t *)0x31000000)
#define HPPS_SMMU_BASE                ((volatile uint32_t *)0xf9300000)

#define TRCH_DMA_BASE   ((volatile uint32_t *)0x21000000)
#define RTPS_DMA_BASE   ((volatile uint32_t *)0x30a08000)

#define LSIO_UART0_BASE ((volatile uint32_t*)0x30000000)
#define LSIO_UART1_BASE ((volatile uint32_t*)0x30001000)

#define HPSC_MBOX_NUM_BLOCKS 3

#define MBOX_LSIO__BASE           ((volatile uint32_t *)0x3000a000)
#define MBOX_HPPS_TRCH__BASE      ((volatile uint32_t *)0xfff50000)
#define MBOX_HPPS_RTPS__BASE      ((volatile uint32_t *)0xfff60000)

#define WDT_TRCH_BASE            ((volatile uint32_t *)0x21002000)

#define WDT_RTPS_TRCH_BASE      ((volatile uint32_t *)0x21003000)
#define WDT_RTPS_RTPS_BASE      ((volatile uint32_t *)0x30a0a000)
#define WDT_RTPS_SIZE           0x1000

#define WDT_HPPS_TRCH_BASE      ((volatile uint32_t *)0x21010000)
#define WDT_HPPS_RTPS_BASE      ((volatile uint32_t *)0xfff70000)
#define WDT_HPPS_SIZE           0x10000

#define SMC_SRAM_BASE           0x28000000
#define SMC_SRAM_SIZE            0x8000000

#endif // HWINFO_H
