cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Match the configuration flags used by the original board projects. Setting
# these before project() prevents CMake's host-oriented defaults (such as -O3
# and -DNDEBUG) from leaking into the embedded builds.
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(STM32Firmware LANGUAGES C CXX ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Remove the incorrect libob.a dependency if C++ is enabled in the future.
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

add_subdirectory(Backplane)
add_subdirectory(ControlBoard)

if ((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(FATAL_ERROR "Firmware requires C11 or higher")
endif ()
