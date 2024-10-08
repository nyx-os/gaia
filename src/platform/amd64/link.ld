/* Tell the linker that we want an Amd64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

/* We want the symbol _start to be our entry point */
ENTRY(_start)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions */
PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    /* We wanna be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    . = 0xffffffff80000000;

    text_start_addr = .;

    .text : {
        *(.text .text.*)
    } :text

    text_end_addr = .;

    /* Move to the next memory page for .rodata */
    . += CONSTANT(MAXPAGESIZE);

    rodata_start_addr = .;

    PROVIDE(symbol_table = 1);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .drivers : {
        drivers_start = .;
        KEEP(*(.drivers*))
        drivers_end = .;
    } :rodata

       /* C++ is a language that allows for global constructors. In order to obtain the */
    /* address of the ".init_array" section we need to define a symbol for it. */
    .init_array : {
        __init_array = .;
        *(.init_array .init_array.*)
        __init_array_end = .;
    } :rodata
    

    rodata_end_addr = .;

    /* Move to the next memory page for .data */
    . += CONSTANT(MAXPAGESIZE);

    data_start_addr = .;

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    data_end_addr = .;

    /* Discard .note.* and .eh_frame since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
