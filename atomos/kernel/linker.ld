/*
 * AtomOS Kernel Linker Script
 * Places kernel at 1MB mark in physical memory
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel loads at 1MB */
    . = 0x100000;

    /* Multiboot header must be first */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot)
    }

    /* Text section (code) */
    .text ALIGN(4K) :
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Kernel end marker */
    _kernel_end = .;

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}
