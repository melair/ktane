add_library(STM32_Common INTERFACE)

# INTERFACE sources are compiled separately by each consuming firmware target,
# so they inherit that board's MCU-specific compiler options.
target_sources(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/fsm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/input_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/input_manager_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/i2c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/i2c_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/spi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/spi_platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/status.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/status.h
)

target_include_directories(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

add_library(STM32_Common_STM32H5 INTERFACE)
target_sources(STM32_Common_STM32H5 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager_platform_stm32h5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c_platform_stm32h5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi_platform_stm32h5.c
)
target_link_libraries(STM32_Common_STM32H5 INTERFACE
        STM32_Common
)

add_library(STM32_Common_STM32G0 INTERFACE)
target_sources(STM32_Common_STM32G0 INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/input_manager_platform_stm32g0.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c_platform_stm32g0.c
)
target_link_libraries(STM32_Common_STM32G0 INTERFACE
        STM32_Common
)
