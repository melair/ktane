#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "../../opts.h"
#include "../../hal/i2c.h"
#include "../../hal/pins.h"
#include "../../tick.h"
#include "../../../common/packet.h"
#include "power_data.h"

/* Local function prototype. */
i2c_command_t *power_i2c_callback(i2c_command_t *cmd);
uint16_t power_bit_number(uint8_t bit_count, uint16_t offset, uint16_t base_bit, uint8_t value);
uint8_t power_get_percentage(uint16_t bat_vol);

#define POWER_BAT_PERCENTAGE_POINTS 21

/* Charge percentage values (5% increments) for LiPo. */
const uint16_t power_charge_percentage[POWER_BAT_PERCENTAGE_POINTS] = {
    3270, // 0%
    3610,
    3690,
    3710,
    3730,
    3750,
    3770,
    3790,
    3800,
    3820,
    3840,
    3850,
    3870,
    3910,
    3950,
    3980,
    4020,
    4080,
    4110,
    4150,
    4200 // 100%
};

void power_initialise(opt_data_t *opt) {
    /* Temp work around until respin, remove once dedicate I2C pins exist.
     * This makes the original I2C pins go HiZ, allowing the soldered
     * connector to act as I2C. */
    kpin_mode(KPORT_BUILD(opt->port, 3), PIN_INPUT, false);
    kpin_mode(KPORT_BUILD(opt->port, 4), PIN_INPUT, false);

    /* Initialise I2C (move to main after respin). */
    i2c_initialise();

    /* Ensure poll stat is set to zero. */
    opt->power.poll_state = 0;
    opt->power.power_off = false;
}

void power_service(opt_data_t *opt) {
    if (tick_2hz && opt->power.poll_state == 0) {
        opt->power.i2c_cmd.addr = MP2723A_ADDRESS;
        opt->power.i2c_cmd.operation = I2C_OPERATION_WRITE;
        opt->power.i2c_cmd.buffer = &opt->power.buffer[0];
        opt->power.i2c_cmd.callback = power_i2c_callback;
        opt->power.i2c_cmd.callback_ptr = (void *) opt;
        opt->power.i2c_cmd.write_size = 2;
        opt->power.i2c_cmd.read_size = 0;

        if (opt->power.power_off) {
            opt->power.power_off = false;
            opt->power.poll_state = 0xff;

            // Force BATFET_DIS.
            opt->power.buffer[0] = 0x0a;
            opt->power.buffer[1] = 0x64;
        } else {
            opt->power.poll_state++;

            // Configure watchdog, enable adc in discharge (sent often))
            // 0x08 0xdd (EN_TERM=1, EN_ADC_DSG=1, WD=40s, WDR=1, CHG_TMR=12h, EN_TIMER=1)

            opt->power.buffer[0] = 0x08;
            opt->power.buffer[1] = 0xdd;
        }

        /* Queue the I2C command. */
        i2c_enqueue(&opt->power.i2c_cmd);
    }

    /* Service I2C (move to main after respin). */
    i2c_service();

    /* After refresh, send data out to network. */
    if (opt->power.poll_state == 8) {
        packet_outgoing.module.power_state.battery_percent = opt->power.adc.bat_percentage;
        packet_outgoing.module.power_state.battery_voltage = opt->power.adc.bat_voltage;
        packet_outgoing.module.power_state.input_voltage = opt->power.adc.input_voltage;
        packet_outgoing.module.power_state.charge_current = opt->power.adc.charge_current;
        packet_outgoing.module.power_state.input_current = opt->power.adc.input_current;
        packet_outgoing.module.power_state.flags.charge_status = opt->power.status.charge_status;
        packet_send(PREFIX_MODULE, OPCODE_MODULE_POWER_STATE, SIZE_MODULE_POWER_STATE, &packet_outgoing);

        opt->power.poll_state = 0;
    }
}

i2c_command_t *power_i2c_callback(i2c_command_t *cmd) {
    opt_data_t *opt = (opt_data_t *) cmd->callback_ptr;

    switch (opt->power.poll_state) {
        case 1:
            // Configure system reset mode
            // 0x00 0x3f (EN_HIZ=0 EN_LIM=0 IIN_LIM=500mA)

            opt->power.poll_state++;
            opt->power.buffer[0] = 0x00;
            opt->power.buffer[1] = 0b00001000;
            return cmd;

        case 2:
            // Configure system reset mode
            // 0x0a 0x40 (SW_FREQ=1.35Mhz, TMR2X_EN=1, BATFET_DIS=0, SYSREST_SEL=0, TDISC_H=2s TDISK_L=8s)

            opt->power.poll_state++;
            opt->power.buffer[0] = 0x0a;
            opt->power.buffer[1] = 0x44;
            return cmd;

        case 3:
            // Configure NTC configuration
            // 0x02 0x50 (TSM_DLY=None, NTC_TYPE=1, EN_OTG_NTC=0, EN_CHG_NTC=1, TJ_REG=60C, NTC_OPT=0, AICO_EN=1)

            opt->power.poll_state++;
            opt->power.buffer[0] = 0x02;
            opt->power.buffer[1] = 0x51;
            return cmd;

        case 4:
            // Configure ADC
            // 0x03 0xd0 (ADC_START=1, ADC_RATE=continuous, VIN_DSCHG=5V, IIN_DSCHG=0.5A)

            opt->power.poll_state++;
            opt->power.buffer[0] = 0x03;
            opt->power.buffer[1] = 0xd0;
            return cmd;

        case 5:
            // Configure ADC
            // 0x06 0x20 (Ipre=230mA, Iterm=120mA)

            opt->power.poll_state++;
            opt->power.buffer[0] = 0x06;
            opt->power.buffer[1] = 0x20;
            return cmd;

        case 6:
            opt->power.poll_state++;
            opt->power.buffer[0] = 0x00;
            opt->power.i2c_cmd.operation = I2C_OPERATION_WRITE_THEN_READ;
            opt->power.i2c_cmd.write_size = 1;
            opt->power.i2c_cmd.read_size = MP2723A_REGISTER_COUNT;
            return cmd;

        case 7:
            opt->power.poll_state++;
            opt->power.status.input_status = (opt->power.buffer[0x0c] >> 5) & 0x07;
            opt->power.status.charge_status = (opt->power.buffer[0x0c] >> 3) & 0x03;
            opt->power.status.input_fault = (opt->power.buffer[0x0d] >> 5) & 0x01;
            opt->power.status.thermal_shutdown = (opt->power.buffer[0x0d] >> 4) & 0x01;
            opt->power.status.bat_fault = (opt->power.buffer[0x0d] >> 3) & 0x01;
            opt->power.status.ntc_fault = opt->power.buffer[0x0d] & 0x07;
            opt->power.status.vinppm_stat = (opt->power.buffer[0x14] >> 7) & 0x01;
            opt->power.status.iinppm_stat = (opt->power.buffer[0x14] >> 6) & 0x01;

            opt->power.adc.bat_voltage = power_bit_number(8, 0, 20, opt->power.buffer[0x0e]);
            opt->power.adc.system_voltage = power_bit_number(8, 0, 20, opt->power.buffer[0x0f]);
            opt->power.adc.input_voltage = power_bit_number(7, 0, 60, opt->power.buffer[0x11]);
            opt->power.adc.charge_current = power_bit_number(8, 0, 175, opt->power.buffer[0x12]) / 10;
            opt->power.adc.input_current = power_bit_number(8, 0, 133, opt->power.buffer[0x13]) / 10;
            opt->power.adc.input_current_limit = power_bit_number(6, 0, 50, opt->power.buffer[0x14]);
            opt->power.adc.bat_percentage = power_get_percentage(opt->power.adc.bat_voltage);
            return NULL;
    }

    return NULL;
}

void power_off(opt_data_t *opt) {
    opt->power.power_off = true;
}

uint16_t power_bit_number(uint8_t bit_count, uint16_t offset, uint16_t base_bit, uint8_t value) {
    uint16_t ret = offset;

    for (uint8_t i = 0; i < bit_count; i++) {
        if ((value & 0x01) == 0x01) {
            ret += base_bit;
        }

        value >>= 1;
        base_bit *= 2;
    }

    return ret;
}

uint8_t power_get_percentage(uint16_t bat_vol) {
    uint8_t i;

    for (i = POWER_BAT_PERCENTAGE_POINTS - 1; i != 0; i--) {
        if (bat_vol >= power_charge_percentage[i]) {
            break;
        }
    }

    if (i == 20) {
        return 100;
    } else if (i == 0 && bat_vol < power_charge_percentage[i]) {
        return 0;
    }

    float pps = (power_charge_percentage[i + 1] - power_charge_percentage[i]) / 5;
    float val = (i * 5) + ((bat_vol - power_charge_percentage[i]) / pps);

    return (uint8_t) floor(val);
}