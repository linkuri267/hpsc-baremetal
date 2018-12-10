#ifndef DRAM_MAP_H
#define DRAM_MAP_H

// This file indicates the resource allocations in DRAM

// Page table should be in memory that is on a bus accessible from the MMUs
// master port ('dma' prop in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging from high-level Chiplet
// diagram.
#define RTPS_HPPS_PT_ADDR 0x8e000000
#define RTPS_HPPS_PT_SIZE   0x200000

// HPPS binary images
#define HPPS_ATF_ADDR      0x80000000
#define HPPS_ATF_SIZE      0x00080000
#define HPPS_BL_ADDR       0x88000000
#define HPPS_BL_SIZE       0x00080000
#define HPPS_KERN_ADDR     0x80080000
#define HPPS_KERN_SIZE     0x00800000
#define HPPS_DT_ADDR       0x84000000
#define HPPS_DT_SIZE       0x00008000
#define HPPS_ROOTFS_ADDR   0x90000000
#define HPPS_ROOTFS_SIZE   0x10000000

// Page tables for RTPS MMU: in RPTS DRAM
#define RTPS_PT_ADDR 0x40010000
#define RTPS_PT_SIZE   0x200000

// DMA Microcode buffer: in RTPS DRAM (cannot be in TCM)
#define RTPS_DMA_MCODE_ADDR 0x40000000
#define RTPS_DMA_MCODE_SIZE 0x00001000

// Buffers for DMA test: in RTPS DRAM
#define RTPS_DMA_SRC_ADDR       0x40001000
#define RTPS_DMA_SIZE           0x00000020
#define RTPS_DMA_DST_ADDR       0x40002000 // align to page
#define RTPS_DMA_DST_REMAP_ADDR 0x40003000 // MMU test maps this to DST_ADDR


#endif // DRAM_MAP_H
