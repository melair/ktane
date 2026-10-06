# Emit flash-only assets, optionally using a basename different from the target.
function(stm32_firmware_assets target output_dir)
    set(asset_name ${target})
    if(ARGC GREATER 2)
        set(asset_name ${ARGV2})
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        BYPRODUCTS ${output_dir}/${asset_name}.hex ${output_dir}/${asset_name}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}> ${output_dir}/${asset_name}.hex
        # Empty TLS sections can report an SRAM LMA in ELF. Converting through
        # HEX prevents --gap-fill from extending a binary into the SRAM range.
        COMMAND ${CMAKE_OBJCOPY} -I ihex -O binary --gap-fill=0xFF
            ${output_dir}/${asset_name}.hex ${output_dir}/${asset_name}.bin
        VERBATIM)
    set_property(TARGET ${target} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        ${output_dir}/${asset_name}.hex ${output_dir}/${asset_name}.bin)
endfunction()
