foreach(board IN ITEMS Backplane Edgework Comms)
    set(application ${board}Application)
    stm32_share_application(${board} ${application})
    if(board STREQUAL "Comms")
        set(base 0x10040000)
        set(flash_end 0x100BF000)
        set(ram_end 0x20010000)
        set(device STM32WB09)
        set(core_path CMSIS_Core/Core/Include)
    else()
        set(base 0x08000000)
        set(flash_end 0x0801F000)
        set(ram_end 0x20009000)
        set(device STM32G070xx)
        set(core_path CMSIS_Core/CMSIS/Core/Include)
    endif()
    math(EXPR application_base "${base} + ${BOOTLOADER_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
    set(boot ${board}Bootloader)
    set(firmware ${board}Firmware)
    set(output_dir ${CMAKE_BINARY_DIR}/${board})
    add_executable(${boot} Src/startup.S Src/bootloader.c Src/version.c)
    target_include_directories(${boot} PRIVATE
        ${PROJECT_SOURCE_DIR}/${board}/App/Inc
        ${PROJECT_SOURCE_DIR}/${board}/Drivers/CMSIS_Device/Include
        ${PROJECT_SOURCE_DIR}/${board}/Drivers/${core_path})
    target_compile_definitions(${boot} PRIVATE ${device}
        $<$<CONFIG:Debug>:DEBUG>)
    target_compile_options(${boot} PRIVATE -mcpu=cortex-m0plus -mthumb
        $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-ffreestanding;-fdata-sections;-ffunction-sections>)
    target_link_libraries(${boot} PRIVATE STM32_Firmware_Version)
    target_link_options(${boot} PRIVATE -mcpu=cortex-m0plus -mthumb -nostdlib
        -T${CMAKE_CURRENT_SOURCE_DIR}/bootloader.ld
        -Wl,--defsym=__bootloader_base=${base}
        -Wl,--defsym=__bootloader_size=${BOOTLOADER_SIZE}
        -Wl,--defsym=__application_base=${application_base}
        -Wl,--defsym=__application_end=${flash_end}
        -Wl,--defsym=__ram_start=0x20000000
        -Wl,--defsym=__ram_end=${ram_end}
        -Wl,--gc-sections -Wl,--print-memory-usage
        -Wl,-Map=${output_dir}/${boot}.map)
    set_target_properties(${boot} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_dir}
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.ld
        ADDITIONAL_CLEAN_FILES ${output_dir}/${boot}.map)

    # A binary object links into a real PT_LOAD segment. Application symbols
    # remain in the combined ELF; GDB loads the original boot ELF separately.
    set(boot_object ${output_dir}/${boot}.o)
    set(boot_entry ${output_dir}/${boot}-entry.ld)
    get_target_property(application_script ${application} LINK_DEPENDS)
    add_custom_command(OUTPUT ${boot_object} ${boot_entry}
        COMMAND ${CMAKE_COMMAND}
            -DOBJCOPY=${CMAKE_OBJCOPY} -DNM=${CMAKE_NM} -DCOMPILER=${CMAKE_C_COMPILER}
            -DBOOT_ELF=$<TARGET_FILE:${boot}>
            -DBOOT_BIN=${output_dir}/${boot}.bin
            -DBOOT_HEX=${output_dir}/${boot}.hex
            -DBOOT_OBJECT=${boot_object} -DENTRY_SCRIPT=${boot_entry}
            -DBASE=${base} -DAPP_BASE=${application_base}
            -DSIZE=${BOOTLOADER_SIZE} -DAPP_SCRIPT=${application_script}
            -P ${PROJECT_SOURCE_DIR}/cmake/assemble_bootloader.cmake
        DEPENDS ${boot} ${PROJECT_SOURCE_DIR}/cmake/assemble_bootloader.cmake
        BYPRODUCTS ${output_dir}/${boot}.bin ${output_dir}/${boot}.hex ${output_dir}/${boot}.o.S
        VERBATIM)
    set_source_files_properties(${boot_object} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    add_executable(${firmware} ${boot_object} $<TARGET_OBJECTS:${board}Objects>)
    foreach(property LINK_OPTIONS LINK_DIRECTORIES LINK_DEPENDS LINK_LIBRARIES)
        get_target_property(value ${application} ${property})
        if(value)
            set_property(TARGET ${firmware} PROPERTY ${property} "${value}")
        endif()
    endforeach()
    # The map must belong to this link rather than overwrite the app's map.
    get_target_property(options ${firmware} LINK_OPTIONS)
    list(TRANSFORM options REPLACE "${application}\\.map" "${firmware}.map")
    list(REMOVE_ITEM options "-T${application_script}")
    set_property(TARGET ${firmware} PROPERTY LINK_OPTIONS "${options}")
    target_link_options(${firmware} PRIVATE -T${boot_entry})
    set_property(TARGET ${firmware} APPEND PROPERTY LINK_DEPENDS ${boot_entry})
    set_target_properties(${firmware} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_dir}
        ADDITIONAL_CLEAN_FILES ${output_dir}/${firmware}.map)
    add_dependencies(${firmware} ${application})
    stm32_firmware_assets(${firmware} ${output_dir} ${board}Combined)
    file(GENERATE OUTPUT ${output_dir}/${firmware}.gdb
        CONTENT "add-symbol-file \"$<TARGET_FILE:${boot}>\"\n")
endforeach()
