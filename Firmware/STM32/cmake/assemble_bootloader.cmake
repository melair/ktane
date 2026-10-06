function(run_tool)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE status)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Bootloader assembly failed: ${ARGV}")
    endif()
endfunction()

run_tool(${OBJCOPY} -O binary --gap-fill=0xFF ${BOOT_ELF} ${BOOT_BIN})
file(SIZE ${BOOT_BIN} binary_size)
if(binary_size GREATER SIZE)
    message(FATAL_ERROR "Bootloader binary does not fit its reservation")
endif()
run_tool(${OBJCOPY} -O ihex ${BOOT_ELF} ${BOOT_HEX})
# Assemble the bytes to retain the compiler's ARM EABI attributes.
file(WRITE ${BOOT_OBJECT}.S
    ".section .bootloader,\"ax\",%progbits\n.incbin \"${BOOT_BIN}\"\n"
    ".section .note.GNU-stack,\"\",%progbits\n")
run_tool(${COMPILER} -mcpu=cortex-m0plus -mthumb -c ${BOOT_OBJECT}.S -o ${BOOT_OBJECT})
execute_process(COMMAND ${NM} --defined-only ${BOOT_ELF}
    OUTPUT_VARIABLE symbols RESULT_VARIABLE status)
if(NOT status EQUAL 0 OR NOT symbols MATCHES "([0-9a-fA-F]+) [Tt] Bootloader_Reset_Handler")
    message(FATAL_ERROR "Cannot find bootloader reset handler")
endif()
math(EXPR reset "0x${CMAKE_MATCH_1} | 1" OUTPUT_FORMAT HEXADECIMAL)
file(WRITE ${ENTRY_SCRIPT}
    "__bootloader_entry = ${reset};\n"
    "MEMORY { BOOT (rx) : ORIGIN = ${BASE}, LENGTH = ${SIZE} }\n"
    "SECTIONS { .bootloader ${BASE} : { KEEP(*(.bootloader)) } >BOOT }\n"
    "INCLUDE \"${APP_SCRIPT}\"\nENTRY(__bootloader_entry)\n"
    "ASSERT(ADDR(.bootloader) + SIZEOF(.bootloader) <= ${APP_BASE}, \"Bootloader overlaps application\")\n")
