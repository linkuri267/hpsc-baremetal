
/* The BM app binary is not relocatable, this is its address in mem */
#ifndef CONFIG_LOAD_ADDR
#define CONFIG_LOAD_ADDR 0x00000000
#endif
/* Leave space for syscfg blob (to support the mode with preloaded syscfg) */
#ifndef CONFIG_LOAD_SIZE
#define CONFIG_LOAD_SIZE 0x000ff000
#endif

MEMORY
{
    SRAM : ORIGIN = CONFIG_LOAD_ADDR, LENGTH = CONFIG_LOAD_SIZE
}

SECTIONS
{
    .text : { *(.text*) } > SRAM
    .rodata : { *(.rodata*) } > SRAM
    .data : { *(.data*) } > SRAM

    /* The GCC linker allocs the explicitly zero-initialized variables
     * into .bss along with uninitialized variables (although C spec says .bss
     * is uninitialized variables). So, the linker assumes that the runtime
     * will zero-initialized the .bss region. I am not sure if COMMON symbols
     * should be zero-initialized, but we will do so. */
    . = ALIGN(4);
    __bss_start__ = .;
    .bss : { *(.bss*) *(COMMON) } > SRAM
    . = ALIGN(4);
    __bss_end__ = .;

    __stacktop = ORIGIN(SRAM) + LENGTH(SRAM) - 4;
}

ENTRY(__entry)
