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

#define HPSC_MBOX_NUM_BLOCKS 3

#define MBOX_LSIO__BASE           ((volatile uint32_t *)0x3000a000)
#define MBOX_HPPS_TRCH__BASE      ((volatile uint32_t *)0xfff50000)
#define MBOX_HPPS_RTPS__BASE      ((volatile uint32_t *)0xfff60000)

#define MBOX_LSIO__IRQ_START              72
#define MBOX_HPPS_TRCH__IRQ_START         136
#define MBOX_HPPS_RTPS__IRQ_START         203

#define TIMER_PHYS_PPI_IRQ              14
#define TIMER_SEC_PPI_IRQ               13
#define TIMER_VIRT_PPI_IRQ              11
#define TIMER_HYP_PPI_IRQ               10

#define WDT_PPI_IRQ                      8

#define WDT_TRCH_BASE            ((volatile uint32_t *)0x21002000)
#define WDT_TRCH_ST1_IRQ         15
#define WDT_TRCH_ST2_IRQ         16 // TODO: should not exist, since hard reset (but keep for testing)

#define WDT_RTPS0_TRCH_BASE      ((volatile uint32_t *)0x21003000)
#define WDT_RTPS0_RTPS_BASE      ((volatile uint32_t *)0x30a0a000)
#define WDT_RTPS1_TRCH_BASE      ((volatile uint32_t *)0x21004000)
#define WDT_RTPS1_RTPS_BASE      ((volatile uint32_t *)0x30a0b000)

// Stage 1 is hooked up to a PPI IRQ on respective core: WDT_PPI_IRQ
// Stage 2 is hooked up to a normal interrupt input on TRCH NVIC
#define WDT_RTPS0_ST2_IRQ         17
#define WDT_RTPS1_ST2_IRQ         18

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
