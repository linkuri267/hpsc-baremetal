#ifndef MEM_MAP_H
#define MEM_MAP_H

// This file indicates the resource allocations in memory

// Shared memory regions accessible to RTPS
#define RTPS_R52_SHM_ADDR                   0x40260000
#define RTPS_R52_SHM_SIZE                   0x00040000

// Shared memory regions accessible to RTPS
// TRCH client -> RTPS server
#define RTPS_R52_SHM_ADDR__TRCH_RTPS_SEND   0x40260000
#define RTPS_R52_SHM_SIZE__TRCH_RTPS_SEND   0x00008000
#define RTPS_R52_SHM_ADDR__RTPS_TRCH_REPLY  0x40268000
#define RTPS_R52_SHM_SIZE__RTPS_TRCH_REPLY  0x00008000
// RTPS client -> TRCH server
#define RTPS_R52_SHM_ADDR__RTPS_TRCH_SEND   0x40270000
#define RTPS_R52_SHM_SIZE__RTPS_TRCH_SEND   0x00008000
#define RTPS_R52_SHM_ADDR__TRCH_RTPS_REPLY  0x40278000
#define RTPS_R52_SHM_SIZE__TRCH_RTPS_REPLY  0x00008000

// Shared memory regions accessible to RTPS, reserved but not allocated
#define RTPS_R52_SHM_ADDR__FREE             0x40280000
#define RTPS_R52_DDR_SIZE__FREE             0x00020000

// Full HPPS DRAM
#define HPPS_DDR_LOW_ADDR   0x80000000
#define HPPS_DDR_LOW_SIZE   0x40000000

// Boot config (TODO: to be replaced by u-boot env/script)
#define HPPS_BOOT_MODE_ADDR 0x8001fffc

// Shared memory regions accessible to HPPS
#define HPPS_SHM_ADDR 0x87600000
#define HPPS_SHM_SIZE   0x400000

// Shared memory regions for userspace
#define HPPS_SHM_ADDR__TRCH_HPPS 0x87600000
#define HPPS_SHM_SIZE__TRCH_HPPS    0x10000
#define HPPS_SHM_ADDR__HPPS_TRCH 0x87610000
#define HPPS_SHM_SIZE__HPPS_TRCH    0x10000

// Shared memory regions for SSW
#define HPPS_SHM_ADDR__TRCH_HPPS_SSW 0x879f0000
#define HPPS_SHM_SIZE__TRCH_HPPS_SSW    0x08000
#define HPPS_SHM_ADDR__HPPS_TRCH_SSW 0x879f8000
#define HPPS_SHM_SIZE__HPPS_TRCH_SSW    0x08000

// Page table should be in memory that is on a bus accessible from the MMUs
// master port ('dma' prop in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging from high-level Chiplet
// diagram.
#define RTPS_HPPS_PT_ADDR                0x87e00000
#define RTPS_HPPS_PT_SIZE                  0x1ff000

#define RT_MMU_TEST_DATA_LO_ADDR         0x87fff000
#define RT_MMU_TEST_DATA_LO_SIZE           0x001000

#define WINDOWS_TO_40BIT_ADDR            0xc0000000
#define WINDOWS_TO_40BIT_SIZE            0x20000000

// Within WINDOWS_TO_40BIT_ADDR, windows to RT_MMU_TEST_DATA_HI_x
#define RT_MMU_TEST_DATA_HI_0_WIN_ADDR   0xc0000000
#define RT_MMU_TEST_DATA_HI_1_WIN_ADDR   0xc1000000

// Two regions of same size
#define RT_MMU_TEST_DATA_HI_0_ADDR      0x100000000
#define RT_MMU_TEST_DATA_HI_1_ADDR      0x100010000
#define RT_MMU_TEST_DATA_HI_SIZE            0x10000

// Memory for Rapid IO, and windows to it in lo mem
#define RIO_MEM_ADDR                    0x100020000
#define RIO_MEM_WIN_ADDR                 0xc2000000
#define RIO_MEM_SIZE                     0x00010000

// Memory for Rapid IO test
#define RIO_MEM_TEST_ADDR               0x100030000
#define RIO_MEM_TEST_SIZE                0x00020000

// Region in window space for RIO test to create mappings
#define RIO_TEST_WIN_ADDR               0xc2020000
#define RIO_TEST_WIN_SIZE               0x00040000

// Page tables for RTPS MMU: in RPTS DRAM
#define RTPS_PT_ADDR 0x40060000
#define RTPS_PT_SIZE   0x200000

// DMA Microcode buffer: in RTPS DRAM (cannot be in TCM)
#define RTPS_DMA_MCODE_ADDR 0x40050000
#define RTPS_DMA_MCODE_SIZE 0x00001000

// Buffers for DMA test: in RTPS DRAM
#define RTPS_DMA_SRC_ADDR       0x40051000
#define RTPS_DMA_SIZE           0x00000200
#define RTPS_DMA_DST_ADDR       0x40052000 // align to page
#define RTPS_DMA_DST_REMAP_ADDR 0x40053000 // MMU test maps this to DST_ADDR


#endif // MEM_MAP_H
