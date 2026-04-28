#include "power.h"
#include "gpio.h"
#include "hal/i2c.h"
#include "hal/pin.h"
#include <utils/fsm.h>
#include <xc.h>

#define INIT_DELAY 10
#define ENABLE_DELAY 500

#define I2C_FRONT_POT_ADDR 0b01011000
#define I2C_REAR_POT_ADDR 0b01011110

extern power_t front;
extern power_t rear;

extern const fs_t power_state_init;
extern const fs_t power_state_unlock_pot;
extern const fs_t power_state_idle;
extern const fs_t power_state_set_current;
extern const fs_t power_state_active;
extern const fs_t power_state_trip;
extern const fs_t power_state_shutdown;

#define POWER_CURRENT_LEN 21
const uint16_t power_i2c_ilim_values[POWER_CURRENT_LEN] = {
    816, // 0.0A (same as 0.1A)
    816, // 0.1A
    422, // 0.2A
    281, // 0.3A
    209, // 0.4A
    165, // 0.5A
    136, // 0.6A
    115, // 0.7A
    98,  // 0.8A
    86,  // 0.9A
    76,  // 1.0A
    67,  // 1.1A
    60,  // 1.2A
    55,  // 1.3A
    50,  // 1.4A
    45,  // 1.5A
    41,  // 1.6A
    38,  // 1.7A
    35,  // 1.8A
    33,  // 1.9A
    30   // 2.0A
};

power_t front = {.module_detect = GPIO_FRONT_MODULE_DETECT,
                 .efuse_en = GPIO_FRONT_EFUSE_EN,
                 .efuse_flt = GPIO_FRONT_EFUSE_FLT,
                 .pot_i2c = {.addr = I2C_FRONT_POT_ADDR}};

power_t rear = {.module_detect = GPIO_REAR_MODULE_DETECT,
                .efuse_en = GPIO_REAR_EFUSE_EN,
                .efuse_flt = GPIO_REAR_EFUSE_FLT,
                .pot_i2c = {.addr = I2C_REAR_POT_ADDR}};

void power_init(void) {
  fsm_init(&front.fsm, &power_state_init, &front);
  fsm_init(&rear.fsm, &power_state_init, &rear);
}

void power_service(void) {
  fsm_service(&front.fsm);
  fsm_service(&rear.fsm);
}

void power_state_init_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  pin_config(pwr->module_detect, INPUT, CFG_PULLUP);
  pin_config(pwr->efuse_flt, INPUT, 0);
  pin_write(pwr->efuse_en, true);
  pin_config(pwr->efuse_en, OUTPUT, CFG_OPENDRAIN);
  pin_write(pwr->efuse_en, true);

  pwr->pot_i2c.buffer = &pwr->pot_i2c_buffer[0];
  pwr->pot_i2c.callback_data = pwr;

  fsm_transition_in(fsm, &power_state_unlock_pot, INIT_DELAY);
}

const fs_t power_state_init = {.enter = &power_state_init_enter,
                               .next_states = {&power_state_unlock_pot, NULL}};

i2c_transaction_t *power_state_unlock_pot_i2c_callback(i2c_transaction_t *t) {
  power_t *pwr = (power_t *)t->callback_data;

  fsm_transition(&pwr->fsm, &power_state_idle);

  return NULL;
}

void power_state_unlock_pot_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  pwr->pot_i2c.callback = &power_state_unlock_pot_i2c_callback;
  pwr->pot_i2c_buffer[0] = 0x1C;
  pwr->pot_i2c_buffer[1] = 0x02;
  pwr->pot_i2c.write_size = 2;
  pwr->pot_i2c.operation = I2C_OPERATION_WRITE;

  i2c_queue(&pwr->pot_i2c);
}

const fs_t power_state_unlock_pot = {.enter = &power_state_unlock_pot_enter,
                                     .next_states = {&power_state_idle, NULL}};

void power_state_idle_service(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Has a module been plugged in? */
  if (!pin_read(pwr->module_detect)) {
    /* Current limit to 200mA. */
    pwr->current_limit = 20;
    /* This will spin for 100ms, and fsm_transition_in will return false. */
    fsm_transition_in(fsm, &power_state_set_current, ENABLE_DELAY);
  }
}

const fs_t power_state_idle = {.service = &power_state_idle_service,
                               .next_states = {&power_state_set_current, NULL}};

i2c_transaction_t *power_state_set_current_i2c_callback(i2c_transaction_t *t) {
  power_t *pwr = (power_t *)t->callback_data;

  fsm_transition(&pwr->fsm, &power_state_active);

  return NULL;
}

void power_state_set_current_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  uint16_t val = 0x0400 | (power_i2c_ilim_values[pwr->current_limit]);

  pwr->pot_i2c.callback = &power_state_set_current_i2c_callback;
  pwr->pot_i2c_buffer[0] = (val >> 8) & 0xff;
  pwr->pot_i2c_buffer[1] = val & 0xff;
  pwr->pot_i2c.write_size = 2;
  pwr->pot_i2c.operation = I2C_OPERATION_WRITE;

  i2c_queue(&pwr->pot_i2c);
}

const fs_t power_state_set_current = {.enter = &power_state_set_current_enter,
                                      .next_states = {&power_state_active}};

void power_state_active_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Enable the eFuse. */
  pin_write(pwr->efuse_en, false);
}

void power_state_active_service(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Check to see if a fault has occurred. */
  if (!pin_read(pwr->efuse_flt)) {
    fsm_transition(fsm, &power_state_trip);
  } else if (pin_read(pwr->module_detect)) {
    /* Module has been removed. */
    fsm_transition(fsm, &power_state_shutdown);
  }
}

const fs_t power_state_active = {
    .enter = &power_state_active_enter,
    .service = &power_state_active_service,
    .next_states = {&power_state_shutdown, &power_state_trip, NULL}};

void power_state_trip_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Disable the eFuse. */
  pin_write(pwr->efuse_en, true);
}

void power_state_trip_service(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  if (pin_read(pwr->module_detect)) {
    /* Module has been removed. */
    fsm_transition(fsm, &power_state_shutdown);
  }
}

const fs_t power_state_trip = {.enter = &power_state_trip_enter,
                               .service = &power_state_trip_service,
                               .next_states = {&power_state_shutdown, NULL}};

void power_state_shutdown_enter(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Disable the eFuse. */
  pin_write(pwr->efuse_en, true);

  fsm_transition(fsm, &power_state_idle);
}

const fs_t power_state_shutdown = {.enter = &power_state_shutdown_enter,
                                   .next_states = {&power_state_idle, NULL}};