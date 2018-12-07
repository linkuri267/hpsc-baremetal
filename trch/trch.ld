
MEMORY
{
    SRAM : ORIGIN = 0x00000000, LENGTH = 0x100000
}

SECTIONS
{
    .text : { *(.text*) } > SRAM
    .rodata : { *(.rodata*) } > SRAM
    .data : { *(.data*) } > SRAM

    .bss : { *(.bss*) *(COMMON) } > SRAM

    __stacktop = LENGTH(SRAM) - 4;
}

ENTRY(__entry)
