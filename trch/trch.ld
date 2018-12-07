
MEMORY
{
    SRAM : ORIGIN = 0x00000000, LENGTH = 0x100000
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

    __stacktop = LENGTH(SRAM) - 4;
}

ENTRY(__entry)
