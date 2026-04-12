#include "power.h"
#include "gpio.h"
#include "hal/pin.h"
#include <utils/fsm.h>
#include <xc.h>

#define INIT_DELAY 10
#define ENABLE_DELAY 500

extern power_t front;
extern power_t rear;

extern const fs_t power_state_init;
extern const fs_t power_state_idle;
extern const fs_t power_state_active;
extern const fs_t power_state_trip;
extern const fs_t power_state_shutdown;

power_t front = {.fsm = {.ctx = &front, .initial = &power_state_init},
                 .module_detect = GPIO_FRONT_MODULE_DETECT,
                 .efuse_en = GPIO_FRONT_EFUSE_EN,
                 .efuse_flt = GPIO_FRONT_EFUSE_FLT,
                 .pot_i2c_address = I2C_FRONT_POT_ADDR};

power_t rear = {.fsm = {.ctx = &rear, .initial = &power_state_init},
                .module_detect = GPIO_REAR_MODULE_DETECT,
                .efuse_en = GPIO_REAR_EFUSE_EN,
                .efuse_flt = GPIO_REAR_EFUSE_FLT,
                .pot_i2c_address = I2C_REAR_POT_ADDR};

void power_init(void) {
  fsm_init(&front.fsm);
  fsm_init(&rear.fsm);
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

  fsm_transition_in(fsm, &power_state_idle, INIT_DELAY);
}

const fs_t power_state_init = {.enter = &power_state_init_enter,
                         .next_states = {&power_state_idle, NULL}};

void power_state_idle_service(fsm_t *fsm) {
  power_t *pwr = (power_t *)fsm->ctx;

  /* Has a module been plugged in? */
  if (!pin_read(pwr->module_detect)) {
    /* This will spin for 100ms, and fsm_transition_in will return false. */
    fsm_transition_in(fsm, &power_state_active, ENABLE_DELAY);
  }
}

const fs_t power_state_idle = {.service = &power_state_idle_service,
                         .next_states = {&power_state_active, NULL}};

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