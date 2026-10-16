# AGENTS.md for KTANE Firmware

## Environment

* You are building firmware for an STM32 processor, of either the STM32H6, STM32G0 or STM32WB09.
* You must approach writing code with this mindset, performance, memory and flash are limited.
* Using the stack is fine, but it's not that large - big objects will cause Hard Faults.
* For code which provides a reusable service (like SPI, I2C, FAT, I2S, Audio), the caller should provide the memory.

## Testing

* At current there is no testing framework, testing is generally not included in the firmware source tree.
* Adhoc tests may be performed to test implementations, but should be removed before completion.
* A debugger is usually available, if one is required ask the user.

## Coding Style

* A little bit of repeating is acceptable, breaking simple checks like `queue.len >= MAX_QUEUE` is overkill, even if static and inline - logic should be complex before breaking out.
* Unless you can prove input is from a human user, generally you should assume it is safe and valid.
* Asserts are acceptable safeguards for debug builds, though currently are not widely used.

## Framework Features / Pitfalls

* In each project, there is a `Drivers` directory, these contain vendor supplied git submodules - which can not be edited.
  * e.g. `CMSIS_Core`, `CMSIS_Device`, `HAL`, `BLE_Stack`
* `Common/Src/fsm/fsm.c` and `Common/Src/fsm/fsm.h` provide a finite state machine framework, with entry, service and exit callbacks. It should be used to manage any complex states.
  * Flags in an object like `bool is_announcing` is a smell that suggest an FSM **might** be appropriate.
* Make use of `union`'s where appropriate, and you can be confident that the memory will never be simultaneously used - they still need to be vaugely related though.