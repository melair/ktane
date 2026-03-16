# Bootloader

## Description

Initial entry point for all MCUs in the KTANE bomb, used to switch between the main MCU code and a firmware flashing tool.

The bomb architecture allows multiple applications to be installed on one MCU, commonly a main application and a firmware flashing tool.

The application may be selected by setting an non volitile memory location to the number of the program to start.

Additionally, if the device has a boot button and LED then the different applications may be switched via holding the button on application of power to the MCU.

## Code Size and Location

* ROM Location: 0x00 - 0x1ff
* Code Offset: 0x0000

## Boot Selector

| C Macro | Description |
| ----------- | ----------- |
| BOOT_SELECTOR | Enable compilation of interactive application selection. |
| BOOT_SELECTOR_BUTTON_PORTbits | PORT?bits.PORT?? that reads the button status. |
| BOOT_SELECTOR_LED_TRISbit | TRIS?bits.TRIS?? that changes the tristate of the LED pin. |
| BOOT_SELECTOR_LED_LATbit | LAT?bits.LAT?? that sets the output state of the LED pin. |
| BOOT_SELECTOR_LED_ODCONbit | ODCON?bits.ODCON?? that sets the output drain configuration. |
