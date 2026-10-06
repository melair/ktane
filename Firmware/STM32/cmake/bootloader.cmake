# Move application compilation into one object library shared by both ELF links.
function(stm32_share_application board application)
    add_library(${board}Objects OBJECT)
    foreach(property SOURCES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS COMPILE_OPTIONS LINK_LIBRARIES)
        get_target_property(value ${application} ${property})
        if(value)
            set_property(TARGET ${board}Objects PROPERTY ${property} "${value}")
        endif()
    endforeach()
    # Driver/common INTERFACE sources belong only to the object compilation.
    # Preserve actual link libraries and version linker flags on both links.
    get_target_property(libraries ${application} LINK_LIBRARIES)
    list(FILTER libraries EXCLUDE REGEX "^STM32_Common")
    set_property(TARGET ${application} PROPERTY SOURCES "$<TARGET_OBJECTS:${board}Objects>")
    set_property(TARGET ${application} PROPERTY LINK_LIBRARIES "${libraries}")
    target_link_options(${application} PRIVATE "-Wl,--defsym=__bootloader_size=${BOOTLOADER_SIZE}")
endfunction()
