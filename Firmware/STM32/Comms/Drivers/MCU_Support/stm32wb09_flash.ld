/*******************************************************************************
* STM32WB09 generic linker file for GCC
* Main linker variables to control it are:
*
* MEMORY_FLASH_APP_SIZE: define the size of the application in case not all the flash is needed.
* Default value is: 508KB when no OTA firmware upgrade is supported
*
* MEMORY_FLASH_APP_OFFSET: define the offset of the application.
* Default value is: 0 offset
*
* MEMORY_RAM_APP_OFFSET: define the offset in RAM from which variables can be
* allocated.
*
*******************************************************************************/

/*******************************************************************************
* Memory Definitions
*******************************************************************************/
/*
STM32WB09 memory map
+-----------------------+ 0x2000FFFF
|  RAM (64K)            |
+-----------------------+ 0x20000000
|                       |
|                       |
+-----------------------+ 0x100BFFFF
|                       |
|  FLASH (512K)         |
+-----------------------+ 0x10040000
|                       |
+-----------------------| 0x100017FF
|   ROM (6K)            |
+-----------------------+ 0x10000000
*/


_MEMORY_RAM_BEGIN_   = 0x20000000;
_MEMORY_RAM_SIZE_    = 0x10000;     /*  64KB  */
_MEMORY_RAM_END_     = 0x2000FFFF;

_MEMORY_FLASH_BEGIN_ = 0x10040000;
_MEMORY_FLASH_SIZE_  = 0x80000;          /* 512KB */
_MEMORY_FLASH_END_   = 0x100BFFFF;

_MEMORY_ROM_BEGIN_   = 0x10000000;
_MEMORY_ROM_SIZE_    = 0x01800;          /* 6KB */
_MEMORY_ROM_END_     = 0x100017FF;


/* Reserved for BTLE stack non volatile memory */
FLASH_NVM_DATASIZE   = (4*1024);


MEMORY_FLASH_APP_OFFSET = DEFINED(MEMORY_FLASH_APP_OFFSET) ? (MEMORY_FLASH_APP_OFFSET) : (0) ;
MEMORY_FLASH_APP_SIZE = DEFINED(MEMORY_FLASH_APP_SIZE) ? (MEMORY_FLASH_APP_SIZE) : (_MEMORY_FLASH_SIZE_ - FLASH_NVM_DATASIZE - MEMORY_FLASH_APP_OFFSET);


/* Entry Point */
ENTRY(Reset_Handler)


/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x000;      /* required amount of heap  */
_Min_Stack_Size = 0xC00; /* required amount of stack */

/* Application addresses are relative to the vector-table base, not FLASH. */
__application_base = _MEMORY_FLASH_BEGIN_ + MEMORY_FLASH_APP_OFFSET;
__application_flash_size = MEMORY_FLASH_APP_SIZE;

/* Specify the memory areas */
MEMORY
{
  RAM (xrw)              : ORIGIN = _MEMORY_RAM_BEGIN_, LENGTH = _MEMORY_RAM_SIZE_ - _Min_Stack_Size
  STACK (xrw)            : ORIGIN = _MEMORY_RAM_BEGIN_ + _MEMORY_RAM_SIZE_ - _Min_Stack_Size, LENGTH = _Min_Stack_Size
  FLASH_PRE (rx)         : ORIGIN = __application_base, LENGTH = 0x300
  FLASH_VERSION (r)      : ORIGIN = __application_base + 0x300, LENGTH = 0x80
  FLASH (rx)             : ORIGIN = __application_base + 0x380, LENGTH = __application_flash_size - 0x380
  REGION_NVM (rx)               : ORIGIN = _MEMORY_FLASH_END_ + 1 - FLASH_NVM_DATASIZE, LENGTH = FLASH_NVM_DATASIZE
  REGION_ROM (rx)               : ORIGIN = _MEMORY_ROM_BEGIN_, LENGTH = _MEMORY_ROM_SIZE_
}


ASSERT(__application_flash_size >= 0x380, "Application flash too small")
ASSERT(ORIGIN(FLASH_PRE) + LENGTH(FLASH_PRE) == ORIGIN(FLASH_VERSION), "Flash gap boundary")
ASSERT(ORIGIN(FLASH_VERSION) + LENGTH(FLASH_VERSION) == ORIGIN(FLASH), "Main flash boundary")
ASSERT(ORIGIN(FLASH) + LENGTH(FLASH) == __application_base + __application_flash_size,
       "Application flash end changed")
ASSERT(ORIGIN(FLASH) + LENGTH(FLASH) <= ORIGIN(REGION_NVM), "FLASH overlaps REGION_NVM")

/* Define output sections */
SECTIONS
{

  /* The startup code goes first into FLASH */
  .intvec __application_base :
  {
    . = ALIGN(4);

    KEEP(*(.intvec)) /* Startup code */

    . = ALIGN(4);
  } >FLASH_PRE

  /* ld places whole ordinary input sections here when they fit; matching
   * patterns in .text/.rodata below receive the remaining sections. */
  .flash_gap :
  {
    *(.text .text* i.* .constdata)
  } >FLASH_PRE

  /* A separate output lets constants try the space left after code spills. */
  .flash_gap_rodata :
  {
    /* Mergeable strings can change size during ld relaxation and must have
     * a single placement. Ordinary constants remain eligible for this gap. */
    INPUT_SECTION_FLAGS (!SHF_MERGE) *(.rodata .rodata*)
  } >FLASH_PRE

  .firmware_version ORIGIN(FLASH_VERSION) (READONLY (TYPE = SHT_PROGBITS)) :
  {
    FILL(0xFFFFFFFF)
    KEEP(*(.firmware_version .firmware_version.*))
    __firmware_version_contents_end = .;
    . = ORIGIN(FLASH_VERSION) + LENGTH(FLASH_VERSION);
  } >FLASH_VERSION

  ASSERT(ADDR(.intvec) == __application_base, "Vectors must start at application base")
  ASSERT(SIZEOF(.intvec) <= 0x300, "Vectors exceed offset 0x300")
  ASSERT(ADDR(.flash_gap) + SIZEOF(.flash_gap) <= __application_base + 0x300,
         "Pre-descriptor sections exceed flash boundary")
  ASSERT(ADDR(.flash_gap_rodata) + SIZEOF(.flash_gap_rodata) <= __application_base + 0x300,
         "Pre-descriptor constants exceed flash boundary")
  ASSERT(__firmware_version_contents_end - ADDR(.firmware_version) <= 0x80,
         "Firmware descriptor contents exceed 128 bytes")
  ASSERT(ADDR(.firmware_version) == __application_base + 0x300,
         "Firmware descriptor must be at application offset 0x300")
  ASSERT(firmware_version == ADDR(.firmware_version), "Firmware object must start descriptor")
  ASSERT(SIZEOF(.firmware_version) == 0x80, "Firmware descriptor must reserve 128 bytes")
  ASSERT(ADDR(.text) == __application_base + 0x380, "Main code must start at offset 0x380")


  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);

    KEEP(*(TAG_OTA_START))

    KEEP(*(.cmd_call_table))
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(i.*)             /* i.* sections (code) */
    *(.constdata)
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
     _etext = .;
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab*)
  } >FLASH

  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* RAM preamble, uninitialized */
  .ram_preamble 0x20000004 (NOLOAD) :
  {
    KEEP(*(.ram_vr))
  }  >RAM

  /* RAM preamble, unininitialized */
  .ram_preamble_2 0x20000034 (NOLOAD) :
  {
    KEEP(*(.crash_info_ram_vr))
  }  >RAM
  /* Uninitialized data section */

  .bss.blueRAM 0x200000C0 :
  {
    . = ALIGN(4);
    _sbssblue = .;         /* define a global symbol at .bss.blueRAM start */
    KEEP(*(.bss.__blue_RAM))
    . = ALIGN(4);
    _ebssblue = .;         /* define a global symbol at .bss.blueRAM end */
  } >RAM

  /* Uninitialized TLS data section */
  .tbss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE( __tbss_end = . );
  } >RAM

  PROVIDE( __tbss_start = ADDR(.tbss) );
  PROVIDE( __tbss_size = __tbss_end - __tbss_start );
  PROVIDE( __tbss_offset = ADDR(.tbss) - ADDR(.tdata) );

  PROVIDE( __tls_base = __tdata_start );
  PROVIDE( __tls_end = __tbss_end );
  PROVIDE( __tls_size = __tls_end - __tls_base );
  PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
  PROVIDE( __tls_size_align = (__tls_size + __tls_align - 1) & ~(__tls_align - 1) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __arm64_tls_tcb_offset = MAX(16, __tls_align) );

  .bss DEFINED(MEMORY_RAM_APP_OFFSET) ? (ORIGIN(RAM) + MEMORY_RAM_APP_OFFSET) : . :
  {
    . = ALIGN(4);
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
      PROVIDE( __bss_end = .);
  } >RAM
  PROVIDE( __non_tls_bss_start = ADDR(.bss) );

  PROVIDE( __bss_start = __tbss_start );
  PROVIDE( __bss_size = __bss_end - __bss_start );


  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
  } >RAM AT> FLASH

  /* Initialized TLS data section */
  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
    PROVIDE(__data_end = .);
    PROVIDE(__tdata_end = .);
  } >RAM AT> FLASH

  PROVIDE( __tdata_start = ADDR(.tdata) );
  PROVIDE( __tdata_size = __tdata_end - __tdata_start );

  PROVIDE( __data_start = ADDR(.data) );
  PROVIDE( __data_size = __data_end - __data_start );

  PROVIDE( __tdata_source = LOADADDR(.tdata) );
  PROVIDE( __tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata) );
  PROVIDE( __tdata_source_size = __tdata_source_end - __tdata_source );

  PROVIDE( __data_source = LOADADDR(.data) );
  PROVIDE( __data_source_end = __tdata_source_end );
  PROVIDE( __data_source_size = __data_source_end - __data_source );


  /* Data section that will not be initialized to any value. */
  .noinit (NOLOAD):
  {
    . = ALIGN(4);
    *(.noinit)
    . = ALIGN(4);
  } >RAM

  .ota_region_end :
  {
	. = ALIGN(4);
	KEEP(*(TAG_OTA_END))
	. = ALIGN(4);
  } >FLASH

  .heap (NOLOAD):
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = ALIGN(4);
  } >RAM

  .stack (NOLOAD) :
  {
    . = ALIGN(4);
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
    _estack = .;         /* define a global symbol at stack end */
    . = ALIGN(4);
  } > STACK


  .rom_info (NOLOAD) :
  {
    . = ALIGN(4);
    KEEP(*(.rom_info))
    . = ALIGN(4);
  } >REGION_ROM

  /* Include gaps, RAM/TLS load copies, and the optional trailing OTA tag. */
  __application_image_end = MAX(ADDR(.ota_region_end) + SIZEOF(.ota_region_end),
                               MAX(LOADADDR(.data) + SIZEOF(.data),
                                   LOADADDR(.tdata) + SIZEOF(.tdata)));
  __application_image_length = ABSOLUTE(__application_image_end - __application_base);
  ASSERT(__application_image_length >= 0x380 &&
         __application_image_length <= __application_flash_size, "Invalid image extent")

  ASSERT(LOADADDR(.data) >= ORIGIN(FLASH), "Data load address precedes main flash")
  ASSERT(LOADADDR(.tdata) + SIZEOF(.tdata) <= ORIGIN(FLASH) + LENGTH(FLASH),
         "Initialized data exceeds application flash")

  /* Final unbounded diagnostic match for live sections rejected by bounded
   * regions. INFO keeps this sink out of load segments; any contents fail the
   * assertion rather than disappearing under non-contiguous placement.
   * Do not restore
   * the Cube library /DISCARD/ rule: it could swallow overflowing live code. */
  .flash_overflow 0 (INFO) :
  {
    *(.text .text* i.* .constdata)
    INPUT_SECTION_FLAGS (!SHF_MERGE) *(.rodata .rodata*)
    *(.glue_7 .glue_7t .eh_frame .init .fini)
    *(.ARM.extab* .gnu.linkonce.armextab.* .ARM.exidx*)
    *(.preinit_array* .init_array* .fini_array*)
    *(.data .data* .RamFunc .RamFunc* .tdata .tdata.* .gnu.linkonce.td.*)
    *(.isr_vector .intvec .firmware_version .firmware_version.*)
  }
  ASSERT(SIZEOF(.flash_overflow) == 0, "Live sections overflow application flash")

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
