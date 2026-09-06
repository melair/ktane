add_library(STM32_Common INTERFACE)

# INTERFACE sources are compiled separately by each consuming firmware target,
# so they inherit that board's MCU-specific compiler options.
target_sources(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/cobs/cobs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/cobs/cobs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/edgework_bus/protocol.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/fsm/fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/fsm/fsm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager/input_manager.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/input_manager/input_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/input_manager/input_manager_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c/i2c.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/i2c/i2c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/i2c/i2c_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/node_link/protocol.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/nvm/nvm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/nvm/nvm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/nvm/nvm_internal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi/spi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/spi/spi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/spi/spi_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/status/status.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/status/status.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/uart/uart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/uart/uart.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/uart/uart_platform.h
)

target_include_directories(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

add_library(STM32_Common_STM32H5 INTERFACE)
target_sources(STM32_Common_STM32H5 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager/input_manager_platform_stm32h5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c/i2c_platform_stm32h5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/nvm/nvm_platform_stm32h5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi/spi_platform_stm32h5.c
)
target_link_libraries(STM32_Common_STM32H5 INTERFACE
        STM32_Common
)

add_library(STM32_Common_STM32G0 INTERFACE)
target_sources(STM32_Common_STM32G0 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager/input_manager_platform_stm32g0.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c/i2c_platform_stm32g0.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/nvm/nvm_platform_stm32g0.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi/spi_platform_stm32g0.c
)
target_link_libraries(STM32_Common_STM32G0 INTERFACE
        STM32_Common
)

add_library(STM32_Common_ARGB INTERFACE)
target_sources(STM32_Common_ARGB INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/argb/argb.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/argb/argb.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/argb/argb_platform.h
)
target_link_libraries(STM32_Common_ARGB INTERFACE
        STM32_Common
)

add_library(STM32_Common_ARGB_STM32H5 INTERFACE)
target_sources(STM32_Common_ARGB_STM32H5 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/argb/argb_platform_stm32h5.c
)
target_link_libraries(STM32_Common_ARGB_STM32H5 INTERFACE
        STM32_Common_ARGB
)

add_library(STM32_Common_ARGB_STM32G0 INTERFACE)
target_sources(STM32_Common_ARGB_STM32G0 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/argb/argb_platform_stm32g0.c
)
target_link_libraries(STM32_Common_ARGB_STM32G0 INTERFACE
        STM32_Common_ARGB
)
