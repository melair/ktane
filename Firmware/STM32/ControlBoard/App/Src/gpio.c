#include "gpio.h"

const GPIO_PinDef GPIO_Button_Pin = {BUTTON_Port, BUTTON_Pin};

const GPIO_PinDef GPIO_A_Pins[GPIO_SUBMODULE_PIN_COUNT] = {
    {GPIO_A0_Port, GPIO_A0_Pin},
    {GPIO_A1_Port, GPIO_A1_Pin},
    {GPIO_A2_Port, GPIO_A2_Pin},
    {GPIO_A3_Port, GPIO_A3_Pin},
    {GPIO_A4_Port, GPIO_A4_Pin},
    {GPIO_A5_Port, GPIO_A5_Pin},
    {GPIO_A6_Port, GPIO_A6_Pin},
    {GPIO_A7_Port, GPIO_A7_Pin},
};

const GPIO_PinDef GPIO_B_Pins[GPIO_SUBMODULE_PIN_COUNT] = {
    {GPIO_B0_Port, GPIO_B0_Pin},
    {GPIO_B1_Port, GPIO_B1_Pin},
    {GPIO_B2_Port, GPIO_B2_Pin},
    {GPIO_B3_Port, GPIO_B3_Pin},
    {GPIO_B4_Port, GPIO_B4_Pin},
    {GPIO_B5_Port, GPIO_B5_Pin},
    {GPIO_B6_Port, GPIO_B6_Pin},
    {GPIO_B7_Port, GPIO_B7_Pin},
};

const GPIO_PinDef GPIO_C_Pins[GPIO_SUBMODULE_PIN_COUNT] = {
    {GPIO_C0_Port, GPIO_C0_Pin},
    {GPIO_C1_Port, GPIO_C1_Pin},
    {GPIO_C2_Port, GPIO_C2_Pin},
    {GPIO_C3_Port, GPIO_C3_Pin},
    {GPIO_C4_Port, GPIO_C4_Pin},
    {GPIO_C5_Port, GPIO_C5_Pin},
    {GPIO_C6_Port, GPIO_C6_Pin},
    {GPIO_C7_Port, GPIO_C7_Pin},
};
