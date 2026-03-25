#include "ssd1680.h"
#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "epaper.h"
#include "spi_queue.h"
#include "ssd1680_operations.h"
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

extern const fs_t ssd1680_power_on;
extern const fs_t ssd1680_hw_reset_phase_one;
extern const fs_t ssd1680_hw_reset_phase_two;
extern const fs_t ssd1680_hw_reset_phase_three;
extern const fs_t ssd1680_sw_reset_phase_one;
extern const fs_t ssd1680_sw_reset_phase_two;
extern const fs_t ssd1680_configure;
extern const fs_t ssd1680_configure_wait;
extern const fs_t ssd1680_queue;
extern const fs_t ssd1680_refresh_display;
extern const fs_t ssd1680_refresh_display_wait;
extern const fs_t ssd1680_sleep;

extern const fs_t ssd1680_power_off;

void ssd1680_idle_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (epaper->commited != NULL) {
    fsm_transition(fsm, &ssd1680_power_on);
  }
}

const fs_t ssd1680_idle = {.name = "IDLE",
                           .next_states = {&ssd1680_power_on, NULL},
                           .service = ssd1680_idle_service};

void ssd1680_power_on_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (epaper->pwr != PORTPIN_NONE) {
    pin_write(epaper->pwr, true);
  }

  fsm_transition_in(fsm, &ssd1680_hw_reset_phase_one, 10);
}

const fs_t ssd1680_power_on = {
    .name = "POWER_ON",
    .next_states = {&ssd1680_hw_reset_phase_one, NULL},
    .enter = ssd1680_power_on_enter};

void ssd1680_hw_reset_phase_one_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  pin_write(epaper->reset, false);

  fsm_transition_in(fsm, &ssd1680_hw_reset_phase_two, 10);
}

const fs_t ssd1680_hw_reset_phase_one = {
    .name = "HW RESET 1",
    .next_states = {&ssd1680_hw_reset_phase_two, NULL},
    .enter = ssd1680_hw_reset_phase_one_enter};

void ssd1680_hw_reset_phase_two_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  pin_write(epaper->reset, true);

  fsm_transition_in(fsm, &ssd1680_hw_reset_phase_three, 10);
}

const fs_t ssd1680_hw_reset_phase_two = {
    .name = "HW RESET 2",
    .next_states = {&ssd1680_hw_reset_phase_three, NULL},
    .enter = ssd1680_hw_reset_phase_two_enter};

void ssd1680_hw_reset_phase_three_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (!pin_read(epaper->busy)) {
    fsm_transition(fsm, &ssd1680_sw_reset_phase_one);
  }
}

const fs_t ssd1680_hw_reset_phase_three = {
    .name = "HW RESET 3",
    .next_states = {&ssd1680_sw_reset_phase_one, NULL},
    .service = ssd1680_hw_reset_phase_three_service};

spi_transaction_t *
ssd1680_sw_reset_phase_one_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  fsm_transition(&epaper->fsm, &ssd1680_sw_reset_phase_two);
  return NULL;
}

void ssd1680_sw_reset_phase_one_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  pin_write(epaper->dc, false);

  epaper->cmd_buffer[0] = SSD1680_CMD_SW_RESET;
  epaper->spi_transaction.write_size = 1;
  epaper->spi_transaction.buffer = &epaper->cmd_buffer[0];
  epaper->spi_transaction.callback = &ssd1680_sw_reset_phase_one_spi_callback;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_sw_reset_phase_one = {
    .name = "SW RESET 1",
    .next_states = {&ssd1680_sw_reset_phase_two, NULL},
    .enter = ssd1680_sw_reset_phase_one_enter};

void ssd1680_sw_reset_phase_two_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (!pin_read(epaper->busy)) {
    fsm_transition(fsm, &ssd1680_configure);
  }
}

const fs_t ssd1680_sw_reset_phase_two = {
    .name = "SW RESET 2",
    .next_states = {&ssd1680_configure, NULL},
    .service = ssd1680_sw_reset_phase_two_service};

const uint8_t ROTATION_TABLE[4] = { 0x03, 0x06, 0x00, 0x05 };

#define INIT_DATA_SIZE 16
const spi_queue_t init_data[INIT_DATA_SIZE] = {
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_DRIVER_OUTPUT_CONTROL}
    },
    {
        .data = 1,
        .size = 3,
        .buffer = {0xaa, 0xaa, 0x00} // EPD Height
    },
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_DATA_ENTRY_MODE}
    },
    {.data = 1, .size = 1, .buffer = {0xaa}},
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_SET_RAM_X}
    },
    {
        .data = 1,
        .size = 2,
        .buffer = {0xaa, 0xbb} // 0 = Start, 1 = End
    },
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_SET_RAM_Y}
    },
    {
        .data = 1,
        .size = 4,
        .buffer = {0xaa, 0xaa, 0xbb, 0xbb} // 0-1 = Start, 2-3 = End
    },
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_BORDER_WAVEFORM_CONTROL}
    },
    {.data = 1, .size = 1, .buffer = {0x05}}, // Follow LUT | LUT1

    {
        .data = 0,
        .size = 1,
        .buffer =
            {SSD1680_CMD_DISPLAY_UPDATE_CONTROL_1}
    },
    {.data = 1,
     .size = 2,
     .buffer = {0x00,
                0x80}}, // Red+B/W RAM Normal, Source Output Mode = S8-S167
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_TEMPERATURE_SENSOR_CONTROL}
    },
    {
        .data = 1, .size = 1, .buffer = {0x80} // Internal temperature sensor
    },
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2}},
    {.data = 1,
     .size = 1,
     .buffer = {0xb1}}, // Load temp, load LUT, disable clock.
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_MASTER_ACTIVIATION}},
};

spi_transaction_t *
ssd1680_configure_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  uint16_t height = epaper->height - 1;
  uint8_t width = (epaper->width/8) - 1;

  if (spi_queue_process(&init_data, INIT_DATA_SIZE, epaper->dc,
                        &epaper->spi_transaction, &epaper->phase)) {
    switch (epaper->phase - 1) {
    case 1:
      epaper->spi_transaction.buffer[0] = (uint8_t)(height & 0xff);
      epaper->spi_transaction.buffer[1] = (uint8_t)((height >> 8) & 0x01);
      break;
    case 3:
      epaper->spi_transaction.buffer[0] = ROTATION_TABLE[epaper->rotation];
      break;
    case 5:
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_270) {
        epaper->spi_transaction.buffer[0] = 0x00;
        epaper->spi_transaction.buffer[1] = width;
      } else {
        epaper->spi_transaction.buffer[0] = width;
        epaper->spi_transaction.buffer[1] = 0x00;      
      }
      break;
    case 7:
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_90) {
        epaper->spi_transaction.buffer[0] = 0x00;
        epaper->spi_transaction.buffer[1] = 0x00;
        epaper->spi_transaction.buffer[2] = (uint8_t)(height & 0xff);
        epaper->spi_transaction.buffer[3] = (uint8_t)((height >> 8) & 0x01);
      } else {
        epaper->spi_transaction.buffer[0] = (uint8_t)(height & 0xff);
        epaper->spi_transaction.buffer[1] = (uint8_t)((height >> 8) & 0x01);   
        epaper->spi_transaction.buffer[2] = 0x00;
        epaper->spi_transaction.buffer[3] = 0x00;
      }
      break;
    }

    return spi;
  } else {
    fsm_transition(&epaper->fsm, &ssd1680_configure_wait);
    return NULL;
  }
}

void ssd1680_configure_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback = &ssd1680_configure_enter_spi_callback;

  spi_transaction_t *t =
      ssd1680_configure_enter_spi_callback(&epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_configure = {.name = "CONFIGURE",
                                .next_states = {&ssd1680_configure_wait, NULL},
                                .enter = ssd1680_configure_enter};

void ssd1680_configure_wait_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (!pin_read(epaper->busy)) {
    fsm_transition(fsm, &ssd1680_queue);
  }
}

const fs_t ssd1680_configure_wait = {.name = "CONFIGURE_COMPLETE",
                                     .next_states = {&ssd1680_queue, NULL},
                                     .service = ssd1680_configure_wait_service};

void ssd1680_queue_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (epaper->commited == NULL) {
    fsm_transition(&epaper->fsm, &ssd1680_refresh_display);
    return;
  }

  switch (epaper->commited->operation) {
  case OPERATION_FILL_WHITE:
  case OPERATION_FILL_BLACK:
  case OPERATION_FILL_RED:
    fsm_transition(&epaper->fsm, &ssd1680_operation_fill);
    break;

  default:
    fsm_transition(&epaper->fsm, &ssd1680_queue_return);
    break;
  }
}

const fs_t ssd1680_queue = {
    .name = "QUEUE",
    .next_states = {&ssd1680_refresh_display, &ssd1680_operation_fill, NULL},
    .enter = ssd1680_queue_enter};

void ssd1680_queue_return_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  epaper->commited = epaper->commited->next;
  fsm_transition(&epaper->fsm, &ssd1680_queue);
}

const fs_t ssd1680_queue_return = {.name = "QUEUE_RETURN",
                                   .next_states = {&ssd1680_queue, NULL},
                                   .enter = ssd1680_queue_return_enter};

#define REFRESH_DATA_SIZE 3
const spi_queue_t refresh_data[REFRESH_DATA_SIZE] = {
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2} // Confgiure
    },
    {.data = 1,
     .size = 1,
     .buffer = {0xf7}}, // Enable analog, load temp, display with mode 1,
                        // disable analog, disable clock.
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_MASTER_ACTIVIATION} // Refresh
    }};

spi_transaction_t *ssd1680_refresh_display_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  if (spi_queue_process(&refresh_data, REFRESH_DATA_SIZE, epaper->dc,
                        &epaper->spi_transaction, &epaper->phase)) {
    return spi;
  } else {
    fsm_transition(&epaper->fsm, &ssd1680_refresh_display_wait);
    return NULL;
  }
}

void ssd1680_refresh_display_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback = &ssd1680_refresh_display_callback;

  spi_transaction_t *t =
      ssd1680_refresh_display_callback(&epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_refresh_display = {
    .name = "REFRESH",
    .next_states = {&ssd1680_refresh_display_wait, NULL},
    .enter = ssd1680_refresh_display_enter};

void ssd1680_refresh_display_wait_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (!pin_read(epaper->busy)) {
    fsm_transition(fsm, &ssd1680_sleep);
  }
}

const fs_t ssd1680_refresh_display_wait = {
    .name = "REFRESH_WAIT",
    .next_states = {&ssd1680_sleep, NULL},
    .service = ssd1680_refresh_display_wait_service};

#define SLEEP_DATA_SIZE 2
const spi_queue_t sleep_data[SLEEP_DATA_SIZE] = {
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_DEEP_SLEEP}},
    {.data = 1, .size = 1, .buffer = {0x01}}, // Mode 1
};

spi_transaction_t *ssd1680_sleep_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  if (spi_queue_process(&sleep_data, SLEEP_DATA_SIZE, epaper->dc,
                        &epaper->spi_transaction, &epaper->phase)) {
    return spi;
  } else {
    fsm_transition(&epaper->fsm, &ssd1680_power_off);
    return NULL;
  }
}

void ssd1680_sleep_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback = &ssd1680_sleep_spi_callback;

  spi_transaction_t *t = ssd1680_sleep_spi_callback(&epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_sleep = {.name = "SLEEP",
                            .next_states = {&ssd1680_power_off, NULL},
                            .enter = ssd1680_sleep_enter};

void ssd1680_power_off_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  pin_write(epaper->dc, false);

  if (epaper->pwr != PORTPIN_NONE) {
    pin_write(epaper->pwr, false);
  }

  fsm_transition(fsm, &ssd1680_idle);
}

const fs_t ssd1680_power_off = {.name = "POWER_ON",
                                .next_states = {&ssd1680_idle, NULL},
                                .enter = ssd1680_power_off_enter};