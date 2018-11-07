#ifndef HWINFO_H
#define HWINFO_H

// This file fulfills the role of a device tree

#define RTPS_GIC_BASE   0x30e00000
#define TRCH_NVIC_BASE  0xe000e000

#define RTPS_TRCH_TO_HPPS_SMMU_BASE   ((volatile uint32_t *)0x31100000)
#define RTPS_SMMU_BASE                ((volatile uint32_t *)0x31000000)
#define HPPS_SMMU_BASE                ((volatile uint32_t *)0xf9300000)

#define TRCH_DMA_BASE   ((volatile uint32_t *)0x21000000)
#define TRCH_DMA_ABORT_IRQ  59
#define TRCH_DMA_EV0_IRQ    60

#define RTPS_DMA_BASE   ((volatile uint32_t *)0x30a08000)
#define RTPS_DMA_ABORT_IRQ  61
#define RTPS_DMA_EV0_IRQ    62

#define LSIO_UART0_BASE ((volatile uint32_t*)0x30000000)
#define LSIO_UART1_BASE ((volatile uint32_t*)0x30001000)

#define HPSC_MBOX_NUM_BLOCKS 2

#define LSIO_MBOX_BASE ((volatile uint32_t *)0x3000a000)
#define HPPS_MBOX_BASE ((volatile uint32_t *)0xfff50000)

#define LSIO_MBOX_IRQ_START         72
#define HPPS_MBOX_IRQ_START         136

// From QEMU device tree / HW spec
#define MASTER_ID_TRCH_CPU  0x2d

#define MASTER_ID_RTPS_CPU0 0x2e
#define MASTER_ID_RTPS_CPU1 0x2f

#define MASTER_ID_HPPS_CPU0 0x80
#define MASTER_ID_HPPS_CPU1 0x8d
#define MASTER_ID_HPPS_CPU2 0x8e
#define MASTER_ID_HPPS_CPU3 0x8f
#define MASTER_ID_HPPS_CPU4 0x90
#define MASTER_ID_HPPS_CPU5 0x9d
#define MASTER_ID_HPPS_CPU6 0x9e
#define MASTER_ID_HPPS_CPU7 0x9f

#define MASTER_ID_TRCH_DMA  0x875
#define MASTER_ID_RTPS_DMA  0x876
#define MASTER_ID_HPPS_DMA  0x877
#define MASTER_ID_SRIO0_DMA 0x878
#define MASTER_ID_SRIO1_DMA 0x879

#endif // HWINFO_H
