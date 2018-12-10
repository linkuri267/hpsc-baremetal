#ifndef DRAM_MAP_H
#define DRAM_MAP_H

// This file indicates the resource allocations in DRAM

// Binary images with executables for HPPS
#define HPPS_DDR_LOW_ADDR   0x80000000
#define HPPS_DDR_LOW_SIZE   0x3fe00000 // usable by the HPPS subsystem

// Page table should be in memory that is on a bus accessible from the MMUs
// master port ('dma' prop in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging from high-level Chiplet
// diagram.
#define RTPS_HPPS_PT_ADDR    0xbfe00000
#define RTPS_HPPS_PT_SIZE      0x200000

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
