# Keep Talking and Nobody Explodes - Physical Version

## Introduction

This is my repository for a physical version of Keep Talking and Nobody Explodes that I am working on, it will eventually include all resources including PCB sources/gerbers, SVG laser cut files, firmware and manual.

## Contents

| Directory                    | Purpose                                                      |
| ---------------------------- | ------------------------------------------------------------ |
| ControlFirmware.X            | Unified firmware used on bomb modules, targeting the PIC18F57Q84 microcontroller. MPLAB IDE 5.45 and upwards needed to open project. |
| PCB/Libraries                | Shared KiCad symbol and footprint libraries for all KTANE projects. |
| PCB/ControlBoard             | Control board, uses the PIC18F57Q84 microcontroller.         |
| PCB/Modules/PullUpDown       | Plugin in module for control board, providing pull up/down, high/low side switches and ESD protection. |
| PCB/Others/ClockStrikeRotary | Module display for timer and others that need large time display, contains 4 digit 7 segment display, seven HD107S and a rotary encoder. |
| PCB/Others/LEDMaze           | Module display for maze, 6x6 matrix of HD107S pixels.        |
| PCB/Others/LEDStrip          | Module display for wires and others, 1x6 strip of HD107S pixels. |

