set(CONTROL_BOARD_TARGET ControlBoard)

add_executable(${CONTROL_BOARD_TARGET})

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
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/chassis_bus.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/game.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/indicator.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode_fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/nodes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/protocol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/mcu_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/mcu_load.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/sys_clock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/tick.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/i2s.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/spi_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/can.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/rng.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/rtc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/argb_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/i2c_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/input_manager_config.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/gpio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/nvm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode/puzzle/simon/simon.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode/support/chassis/dac.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode/support/chassis/chassis.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode/support/timer/timer.c
)

set(MCU_SUPPORT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/stm32h5xx_it.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/syscalls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/sysmem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/Src/system_stm32h5xx.c
)

set(CMSIS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS_Device/Source/Templates/gcc/startup_stm32h562xx.s
)

set(HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_adc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_adc_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_fdcan.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_cortex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_dma.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_dma_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_exti.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_flash.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_flash_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_gpio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_i2c.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_i2c_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_i2s.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_icache.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_pwr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_pwr_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rng.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rng_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rcc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rcc_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rtc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_rtc_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_spi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_spi_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_tim.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_tim_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_uart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_uart_ex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_usart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/stm32h5xx_hal_usart_ex.c
)

target_sources(${CONTROL_BOARD_TARGET} PRIVATE
        ${APP_SOURCES}
        ${MCU_SUPPORT_SOURCES}
        ${CMSIS_SOURCES}
        ${HAL_SOURCES}
)

target_include_directories(${CONTROL_BOARD_TARGET} PRIVATE
        ${APP_INCLUDE_DIRS}
        ${MCU_SUPPORT_INCLUDE_DIRS}
        ${HAL_INCLUDE_DIRS}
        ${CMSIS_INCLUDE_DIRS}
)

target_compile_definitions(${CONTROL_BOARD_TARGET} PRIVATE
        USE_HAL_DRIVER
        STM32H562xx
        $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(${CONTROL_BOARD_TARGET} PRIVATE
        -mcpu=cortex-m33
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-fdata-sections;-ffunction-sections;-fstack-usage>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions;-fno-threadsafe-statics>
        $<$<COMPILE_LANGUAGE:ASM>:-x;assembler-with-cpp;-MMD;-MP>
)

target_link_options(${CONTROL_BOARD_TARGET} PRIVATE
        -mcpu=cortex-m33
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
        -T${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/STM32H562xx_FLASH.ld
        --specs=nano.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CONTROL_BOARD_TARGET}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
)

target_link_directories(${CONTROL_BOARD_TARGET} PRIVATE
        # Add user defined library search paths here
)

target_link_libraries(${CONTROL_BOARD_TARGET} PRIVATE
        STM32_Common_STM32H5
        STM32_Common_ARGB_STM32H5
        m
)

set_target_properties(${CONTROL_BOARD_TARGET}
        PROPERTIES
        ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${CONTROL_BOARD_TARGET}.map
)
