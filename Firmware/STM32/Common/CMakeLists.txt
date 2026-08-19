add_library(STM32_Common INTERFACE)

# INTERFACE sources are compiled separately by each consuming firmware target,
# so they inherit that board's MCU-specific compiler options.
target_sources(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src/fsm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc/fsm.h
)

target_include_directories(STM32_Common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)
