
MEMORY
{
    TCM_A (RWX) :         ORIGIN = 0x00000000, LENGTH =   0x100000
    TCM_B (RWX) :         ORIGIN = 0x00100000, LENGTH =   0x100000
    /* TODO: TCM C (not in device tree) */

    PERIPH (RW) :         ORIGIN = 0x30000000, LENGTH = 0x10000000

    /* Split only to place at same place as U-boot (the higher half) */
    RTPS_DDR_LOW_0 (RWX): ORIGIN = 0x40000000, LENGTH = 0x20000000
    RTPS_DDR_LOW_1 (RWX): ORIGIN = 0x70000000, LENGTH = 0x10000000

    /* Just to define symbols for MPU */
    HPPS_DDR_LOW (RW):    ORIGIN = 0x80000000, LENGTH = 0x40000000
    WINDOW (RW):          ORIGIN = 0xc0000000, LENGTH = 0x20000000

    HSIO (RW):            ORIGIN = 0xe3000000, LENGTH = 0x15000000
    HPPS_MBOX (RW):       ORIGIN = 0xfff60000, LENGTH = 0x00010000
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
    } > RTPS_DDR_LOW_1
    .data BLOCK(64) : {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > RTPS_DDR_LOW_1
    .bss BLOCK(64) : {
        __bss_start__ = .;
        *(.bss)
        *(COMMON)
         . = ALIGN(64);
        __bss_end__ = .;
    } > RTPS_DDR_LOW_1
    end = .;
    __stack_start__ = __bss_end__;
    __stack_end__ = ORIGIN(RTPS_DDR_LOW_1) + LENGTH(RTPS_DDR_LOW_1) - 64;

   __tcm_a_start__ = ORIGIN(TCM_A);
   __tcm_a_end__ = ORIGIN(TCM_A) + LENGTH(TCM_A) - 64;
   __tcm_b_start__ = ORIGIN(TCM_B);
   __tcm_b_end__ = ORIGIN(TCM_B) + LENGTH(TCM_B) - 64;
   __rtps_ddr_low_0_start__ = ORIGIN(RTPS_DDR_LOW_0);
   __rtps_ddr_low_0_end__ = ORIGIN(RTPS_DDR_LOW_0) + LENGTH(RTPS_DDR_LOW_0) - 64;
   __periph_start__ = ORIGIN(PERIPH);
   __periph_end__ = ORIGIN(PERIPH) + LENGTH(PERIPH) - 64;
   __hpps_ddr_low_start__ = ORIGIN(HPPS_DDR_LOW);
   __hpps_ddr_low_end__ = ORIGIN(HPPS_DDR_LOW) + LENGTH(HPPS_DDR_LOW) - 64;
   __window_start__ = ORIGIN(WINDOW);
   __window_end__ = ORIGIN(WINDOW) + LENGTH(WINDOW) - 64;
   __hsio_start__ = ORIGIN(HSIO);
   __hsio_end__ = ORIGIN(HSIO) + LENGTH(HSIO) - 64;
   __hpps_mbox_start__ = ORIGIN(HPPS_MBOX);
   __hpps_mbox_end__ = ORIGIN(HPPS_MBOX) + LENGTH(HPPS_MBOX) - 64;
}
