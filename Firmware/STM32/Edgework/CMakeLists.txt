set(EDGEWORK_TARGET Edgework)

add_executable(${EDGEWORK_TARGET})

set(APP_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc
)

set(MCU_SUPPORT_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Inc
)

set(HAL_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Inc/Legacy
)

set(CMSIS_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS_Device/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS_Core/CMSIS/Core/Include
)

set(APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/edgework_bus.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/argb_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/i2c_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/mcu_init.c
)

set(MCU_SUPPORT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/stm32g0xx_hal_msp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/stm32g0xx_it.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/syscalls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/sysmem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/system_stm32g0xx.c
)

set(CMSIS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS_Device/Source/Templates/gcc/startup_stm32g070xx.s
)

set(HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_adc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_adc_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_cortex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_dma.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_dma_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_exti.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_flash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_flash_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_gpio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_i2c.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_i2c_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_pwr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_pwr_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_rcc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_rcc_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_tim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_tim_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_uart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_hal_uart_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_ll_adc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32g0xx_ll_rcc.c
)

target_sources(${EDGEWORK_TARGET} PRIVATE
        ${APP_SOURCES}
        ${MCU_SUPPORT_SOURCES}
        ${CMSIS_SOURCES}
        ${HAL_SOURCES}
)

target_include_directories(${EDGEWORK_TARGET} PRIVATE
        ${APP_INCLUDE_DIRS}
        ${MCU_SUPPORT_INCLUDE_DIRS}
        ${HAL_INCLUDE_DIRS}
        ${CMSIS_INCLUDE_DIRS}
)

target_compile_definitions(${EDGEWORK_TARGET} PRIVATE
        USE_HAL_DRIVER
        STM32G070xx
        $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(${EDGEWORK_TARGET} PRIVATE
        -mcpu=cortex-m0plus
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-fdata-sections;-ffunction-sections;-fstack-usage>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions;-fno-threadsafe-statics>
        $<$<COMPILE_LANGUAGE:ASM>:-x;assembler-with-cpp;-MMD;-MP>
)

target_link_options(${EDGEWORK_TARGET} PRIVATE
        -mcpu=cortex-m0plus
        -T${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/STM32G070XX_FLASH.ld
        --specs=nano.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${EDGEWORK_TARGET}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
)

target_link_directories(${EDGEWORK_TARGET} PRIVATE
        # Add user defined library search paths here
)

target_link_libraries(${EDGEWORK_TARGET} PRIVATE
        STM32_Common_STM32G0
        STM32_Common_ARGB_STM32G0
        m
)

set_target_properties(${EDGEWORK_TARGET}
        PROPERTIES
        ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${EDGEWORK_TARGET}.map
)
