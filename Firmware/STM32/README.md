# KTANE Bomb Operating System

The BOS is based on the STM32 ecosystem, specifically targetting H5, G0 and WB0. These are all kept together so that they may share common headers and code.

## Building with CMake

Open this `STM32` directory as the CMake project in CLion. The root project exposes
the `Backplane` and `ControlBoard` executable targets in both the `Debug` and
`Release` CMake profiles.

The same combinations are available from the command line as build presets:

```shell
cmake --preset Debug
cmake --build --preset Backplane-Debug
cmake --build --preset ControlBoard-Debug

cmake --preset Release
cmake --build --preset Backplane-Release
cmake --build --preset ControlBoard-Release
```

Build products are written below `build/Debug` and `build/Release`, separated by
board. Each board keeps its own sources, HAL/CMSIS includes, MCU flags, and linker
script; only the common `arm-none-eabi` compiler selection is centralised.

## STM32CubeMX

**The original project was generated with STM32CubeMX, however it must never be updated with it again.**

I (@pwood) fundementally disagree with the vendoring of STM's code into the codebase, I do not want the HAL libraries copied, and then to rely on CubeMX to bring in other HAL libraries as we need them.

I also do not like the layout of the default MX project, or having to fit code between specific comment blocks to avoid failed code regeneration. Further, some peripherals and HAL init will need to be done on a case by case basis later in the code path (i.e. once a module understands what it needs to support) - this would be incompatible with the way MX works.

As such, the HAL, CMSIS Core and CMSIS Device libraries are git submodules, and the CMake files have been changed to reference them.

Some files are required to be copied and modified, such as the FLASH and RAM loader scripts. Those files may be regenerated using STM32CubeMX by pointing the output of the .ioc at a different directory, and then copying the patch files in.

Interestingly this is closer to STM32CubeMX2 (currently only for the C5 series) with allows copying of snippets from MX2 to your code base.

### Adding new HAL features

* Generate the code using STM32CubeMX into an empty source tree, using the .ioc.
* Copy the initialisation to logical parts of the BOS code.
* Remember to update `Core/Inc/stm32*_hal_conf.h` as needed to enable the HAL function, as well as att to `cmake/stm32cubemx/CMakeLists.txt`.
