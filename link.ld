ENTRY(start)

OUTPUT_FORMAT(elf32-i386)

KRNL_START = 0x00100000;

SECTIONS
{
    . = KRNL_START;

    __kernel_start      = ALIGN(4K);

    __kernel_code_start = ALIGN(4K);
    .text : ALIGN(4K) {
        *(.text*)
    }
    __kernel_code_end   = ALIGN(4K);

    __kernel_data_start = ALIGN(4K);
    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }
    __kernel_data_end = ALIGN(4K);

    __kernel_bss_start = ALIGN(4K);
    .bss : ALIGN(4K) {
        *(stack)
        *(.bss*)
        *(COMMON)
    }
    __kernel_bss_end = ALIGN(4K);

    __kernel_end = ALIGN(4K);
}