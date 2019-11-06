/* The following defaults assume this:
 * RTPS DDR (R52's partition): 512MB                0x20000000
 * u-boot                      0x40000000           0x00040000
 * app image                   0x40040000           0x00010000
 */

/* MMU tables, DMA microcode, SHMEM regions, etc */
#ifndef CONFIG_DRAM_DMA_ADDR
#define CONFIG_DRAM_DMA_ADDR 0x40050000
#endif
#ifndef CONFIG_DRAM_DMA_SIZE
#define CONFIG_DRAM_DMA_SIZE 0x00260000
#endif

/* The BM app binary is not relocatable, this is its address in mem */
#ifndef CONFIG_LOAD_ADDR
#define CONFIG_LOAD_ADDR 0x40300000
#endif
#ifndef CONFIG_LOAD_SIZE
#define CONFIG_LOAD_SIZE 0x1fd00000
#endif

MEMORY
{
    TCM_A (RWX) :         ORIGIN = 0x00000000, LENGTH =   0x100000
    TCM_B (RWX) :         ORIGIN = 0x00100000, LENGTH =   0x100000
    /* TODO: TCM C (not in device tree) */

    PERIPH (RW) :         ORIGIN = 0x30000000, LENGTH = 0x10000000

    RTPS_DDR_DMA (RWX):   ORIGIN = CONFIG_DRAM_DMA_ADDR,
                          LENGTH = CONFIG_DRAM_DMA_SIZE
    RTPS_DDR_OS (RWX):    ORIGIN = CONFIG_LOAD_ADDR, LENGTH = CONFIG_LOAD_SIZE

    /* Just to define symbols for MPU */
    HPPS_DDR_LOW (RW):    ORIGIN = 0x80000000, LENGTH = 0x40000000
    WINDOW (RW):          ORIGIN = 0xc0000000, LENGTH = 0x20000000

    HSIO (RW):            ORIGIN = 0xe3000000, LENGTH = 0x15000000
    HPPS_MBOX (RW):       ORIGIN = 0xfff60000, LENGTH = 0x00010000
    HPPS_UART (RW):       ORIGIN = 0xf92c0000, LENGTH = 0x00010000
}

/* All the start/end symbols need to be 64-byte aligned for MPU */
SECTIONS
{
    .text : { 
         __text_start__ = .;
         *(.text*) 
         *(.init*) 
         *(.fini) 
         *(.rodata*) /* Could define a separate MPU region for RO data */
         . = ALIGN(64);
         __text_end__ = .;
    } > RTPS_DDR_OS
    .data BLOCK(64) : {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > RTPS_DDR_OS
    .bss BLOCK(64) : {
        __bss_start__ = .;
        *(.bss)
        *(COMMON)
         . = ALIGN(64);
        __bss_end__ = .;
    } > RTPS_DDR_OS
    end = .;
    __stack_start__ = __bss_end__;
    __stack_end__ = ORIGIN(RTPS_DDR_OS) + LENGTH(RTPS_DDR_OS) - 64;

   __tcm_a_start__ = ORIGIN(TCM_A);
   __tcm_a_end__ = ORIGIN(TCM_A) + LENGTH(TCM_A) - 64;
   __tcm_b_start__ = ORIGIN(TCM_B);
   __tcm_b_end__ = ORIGIN(TCM_B) + LENGTH(TCM_B) - 64;
   __periph_start__ = ORIGIN(PERIPH);
   __periph_end__ = ORIGIN(PERIPH) + LENGTH(PERIPH) - 64;
   __hpps_ddr_low_start__ = ORIGIN(HPPS_DDR_LOW);
   __hpps_ddr_low_end__ = ORIGIN(HPPS_DDR_LOW) + LENGTH(HPPS_DDR_LOW) - 64;
   __hsio_start__ = ORIGIN(HSIO);
   __hsio_end__ = ORIGIN(HSIO) + LENGTH(HSIO) - 64;
   __hpps_mbox_start__ = ORIGIN(HPPS_MBOX);
   __hpps_mbox_end__ = ORIGIN(HPPS_MBOX) + LENGTH(HPPS_MBOX) - 64;

   /* When running in SPLIT mode, we don't have enough UARTs in LSIO... */
   __hpps_uart_start__ = ORIGIN(HPPS_UART);
   __hpps_uart_end__ = ORIGIN(HPPS_UART) + LENGTH(HPPS_UART) - 64;

   __rtps_ddr_dma_start__ = ORIGIN(RTPS_DDR_DMA);
   __rtps_ddr_dma_end__ = ORIGIN(RTPS_DDR_DMA) + LENGTH(RTPS_DDR_DMA) - 64;
   __window_start__ = ORIGIN(WINDOW);
   __window_end__ = ORIGIN(WINDOW) + LENGTH(WINDOW) - 64;
}
