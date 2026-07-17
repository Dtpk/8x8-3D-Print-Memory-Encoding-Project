/* Minimal GBA Linker Script */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    rom : ORIGIN = 0x08000000, LENGTH = 32M
    iwram : ORIGIN = 0x03000000, LENGTH = 32K
    ewram : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS {
    .text : {
        KEEP(*(.text.start))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > rom

    .data : {
        *(.data .data.*)
    } > iwram AT > rom

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } > iwram

    __data_load = LOADADDR(.data);
    __data_start = ADDR(.data);
    __data_end = ADDR(.data) + SIZEOF(.data);
    __bss_start = ADDR(.bss);
    __bss_end = ADDR(.bss) + SIZEOF(.bss);
}
