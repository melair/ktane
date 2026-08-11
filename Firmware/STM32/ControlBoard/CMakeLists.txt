cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_PROJECT_NAME ControlBoard)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

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
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/indicator.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/module.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/module_fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/protocol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/status.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/mcu_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/mcu_load.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/tick.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/i2c.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/spi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/can.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/rng.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/rtc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/argb.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/cbus.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/input_manager.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/gpio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/sys/nvm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/mode/puzzle/simon/simon.c
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

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${APP_SOURCES}
        ${MCU_SUPPORT_SOURCES}
        ${CMSIS_SOURCES}
        ${HAL_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${APP_INCLUDE_DIRS}
        ${MCU_SUPPORT_INCLUDE_DIRS}
        ${HAL_INCLUDE_DIRS}
        ${CMSIS_INCLUDE_DIRS}
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        USE_HAL_DRIVER
        STM32H562xx
        $<$<CONFIG:Debug>:DEBUG>
)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths here
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ${TOOLCHAIN_LINK_LIBRARIES}
)

set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
        ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map
)

if ((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(FATAL_ERROR "Generated code requires C11 or higher")
endif ()
