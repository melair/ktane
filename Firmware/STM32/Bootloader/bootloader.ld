ENTRY(Bootloader_Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = __bootloader_base, LENGTH = __bootloader_size
}

SECTIONS
{
    __vector_alignment = __vector_table_size <= 128 ? 128 : 256;
    .isr_vector __bootloader_base : { KEEP(*(.isr_vector)) } >FLASH
    __vector_table_offset = ABSOLUTE(ADDR(.isr_vector) - __bootloader_base);
    .firmware_version (__bootloader_base + 0x100) (READONLY (TYPE = SHT_PROGBITS)) :
    {
        KEEP(*(.firmware_version))
    } >FLASH
    __code_offset = ABSOLUTE(ADDR(.text) - __bootloader_base);
    .text (__bootloader_base + 0x120) :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } >FLASH
    .ARM.extab : { *(.ARM.extab*) } >FLASH
    .ARM.exidx : { *(.ARM.exidx*) } >FLASH
    __application_image_length = ABSOLUTE(. - __bootloader_base);

    /* Startup intentionally has no data copy or BSS initialization. */
    .unsupported 0 (INFO) : { *(.data .data.* .bss .bss.* COMMON) }
    ASSERT(SIZEOF(.unsupported) == 0, "Bootloader requires writable-global startup support")
    ASSERT(SIZEOF(.isr_vector) == __vector_table_size, "Bootloader vector table size changed")
    ASSERT((ADDR(.isr_vector) & (__vector_alignment - 1)) == 0, "Bootloader vectors misaligned")
    ASSERT(firmware_version == __bootloader_base + 0x100, "Bootloader descriptor moved")
    ASSERT(SIZEOF(.firmware_version) == 18, "Bootloader descriptor layout changed")
    ASSERT(SIZEOF(.isr_vector) <= 0x100, "Bootloader vectors overlap descriptor")
    ASSERT(__code_offset == 0x120, "Bootloader code offset changed")
    ASSERT(SIZEOF(.firmware_version) <= 0x20, "Bootloader descriptor overlaps code")
    ASSERT(__application_image_length <= __bootloader_size, "Bootloader exceeds reservation")
}
