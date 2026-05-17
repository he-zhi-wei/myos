ENTRY(_start)

SECTIONS {
    . = 1M;    /* kernel loaded at 1MB */

    .text : {
        *(.multiboot)   /* multiboot header must come first */
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
