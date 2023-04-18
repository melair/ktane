#ifndef POWER_DATA_H
#define	POWER_DATA_H

#include "../../hal/i2c.h"

#define MP2723A_ADDRESS         0x4b
#define MP2723A_REGISTER_COUNT  0x18

#define MP2723A_INPUT_NONE      0b000
#define MP2723A_INPUT_UNKNOWN   0b001
#define MP2723A_INPUT_APPLE1_0A 0b010
#define MP2723A_INPUT_APPLE2_1A 0b011
#define MP2723A_INPUT_APPLE2_4A 0b100
#define MP2723A_INPUT_SDP       0b101
#define MP2723A_INPUT_CDP       0b110
#define MP2723A_INPUT_DCP       0b111

#define MP2723A_CHARGE_NONE             0b00
#define MP2723A_CHARGE_TRICKLE          0b01
#define MP2723A_CHARGE_CONSTANT_CURRENT 0b10
#define MP2723A_CHARGE_COMPLETE         0b11

#define MP2723A_NTC_FAULT_NORMAL   0b000
#define MP2723A_NTC_FAULT_NTC_WARM 0b010
#define MP2723A_NTC_FAULT_NTC_COOL 0b011
#define MP2723A_NTC_FAULT_NTC_COLD 0b101
#define MP2723A_NTC_FAULT_NTC_HOT  0b110

typedef struct {
    uint8_t buffer[MP2723A_REGISTER_COUNT];
    i2c_command_t i2c_cmd;
    
    uint8_t poll_state;
    bool power_off;
    
    struct {
        unsigned input_status :3;
        unsigned charge_status :2;
        unsigned input_fault :1;
        unsigned thermal_shutdown :1;
        unsigned bat_fault :1;
        unsigned ntc_fault :3;
        unsigned vinppm_stat :1;
        unsigned iinppm_stat :1;
    } status;
    
    struct {
        uint16_t bat_voltage;
        uint8_t bat_percentage;
        uint16_t system_voltage;
        uint16_t input_voltage;
        uint16_t charge_current;
        uint16_t input_current;
        uint16_t input_current_limit;
    } adc;
} opt_power_t;


#endif	/* POWER_DATA_H */

