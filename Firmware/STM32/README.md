# KTANE Bomb Operating System

## Build and upload

Open this directory in CLion. Select a `<Board>Firmware` run configuration and
the `Debug` or `Release` CMake profile. Run/Debug builds and uploads the ELF.

| Board | Firmware target | ST-LINK debug profile |
| --- | --- | --- |
| Backplane | `BackplaneFirmware` | `ST-LINK (STM32G0)` |
| Edgework | `EdgeworkFirmware` | `ST-LINK (STM32G0)` |
| ControlBoard | `ControlBoardFirmware` | `ST-LINK (STM32H5)` |
| Comms | `CommsFirmware` | `ST-LINK (STM32WB09)` |

If profiles are missing, reload CMake and enable them under Settings → Build,
Execution, Deployment → CMake. Configure CubeCLT server/programmer paths under
Debugger → Debug Profiles; the MCU profiles serve both build configurations.

From the command line:

```shell
cmake --preset Debug
cmake --build build/Debug --target BackplaneFirmware
```

Substitute another target from the table or use `Release` in both commands.
Outputs are in `build/<configuration>/<Board>/`:

- Backplane, Edgework and Comms: `<Board>Firmware.elf`, `<Board>Combined.hex`
  and `<Board>Combined.bin` contain the bootloader and application.
- Their `<Board>Application` targets produce application-only `.elf`, `.hex`
  and `.bin` files; these require a compatible bootloader already installed.
- `<Board>Bootloader` targets build standalone ELFs. Firmware builds also emit
  bootloader `.hex` and unpadded `.bin` files.
- ControlBoard has no separate bootloader; its outputs are
  `ControlBoardFirmware.elf`, `.hex`, `.bin` and `.map`.

HEX and ELF carry their addresses. Upload combined binaries at the bootloader
base and application-only binaries at the application base. NVM is excluded.

| Board | Bootloader base | Application base | NVM (4 KB) |
| --- | --- | --- | --- |
| Backplane | `0x08000000` | `0x08002000` | `0x0801F000` |
| Edgework | `0x08000000` | `0x08002000` | `0x0801F000` |
| Comms | `0x10040000` | `0x10042000` | `0x100BF000` |

## Bootloader and image format

Backplane, Edgework and Comms reserve 8 KB for the bootloader. It validates the
application descriptor and MSP/reset vector, then enters the reset handler with
interrupts masked. Invalid applications halt in `Bootloader_InvalidApplication`.
Application startup installs VTOR and enables interrupts. ControlBoard boots
directly into its firmware.

Each image starts with its complete vector table. Offsets below are bytes from
the standalone image's base:

| Images | Vector offset | Version offset | Code offset |
| --- | --- | --- | --- |
| Cortex-M0+ bootloaders and applications | `0x000` | `0x100` | `0x120` |
| Cortex-M33 application | `0x000` | `0x300` | `0x320` |

The packed version descriptor is 18 bytes, followed by 14 bytes of padding.
Its `image_length` covers the full flash image, including initialised RAM data;
CRC fields remain uncalculated. Combined binaries fill the unused bootloader
reservation with `0xFF`.

Change Backplane, Edgework and Comms platform/hardware identity in
`<Board>/App/Inc/board_identity.h`. Software versions and image types are defined
in each image's `version.c`. Startup assembly lives in
`<Board>/Drivers/MCU_Support/Src/`; keep vendor `CMSIS_Device` submodules unmodified.

Rebuild and upload bootloader and application together after changing the
descriptor layout; images using different layouts cannot be mixed.

To load bootloader symbols alongside the application, source the generated
script in CLion's GDB console:

```gdb
source "/path/to/STM32/build/Debug/<Board>/<Board>Firmware.gdb"
```

Substitute the board and configuration being debugged.