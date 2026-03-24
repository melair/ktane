#include "ssd1680.h"
#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "epaper.h"
#include <stdbool.h>
#include <xc.h>

extern const fs_t ssd1680_power_on;
extern const fs_t ssd1680_hw_reset_phase_one;
extern const fs_t ssd1680_hw_reset_phase_two;
extern const fs_t ssd1680_hw_reset_phase_three;
extern const fs_t ssd1680_sw_reset_phase_one;
extern const fs_t ssd1680_sw_reset_phase_two;
extern const fs_t ssd1680_configure;

extern const fs_t ssd1680_power_off;

void ssd1680_idle_service(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (epaper->commited != NULL) {
    fsm_transition(fsm, &ssd1680_power_on);
  }

  // TODO: TEMP TESTING
  epaper->commited = NULL;
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

  epaper->cmd_buffer[0] = 0x92;
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
    .next_states = {&ssd1680_configure, NULL },
                    .service = ssd1680_sw_reset_phase_two_service };

typedef struct {
  unsigned data : 1;
  unsigned size : 3;
  uint8_t buffer[4];
} init_option_t;

#define INIT_DATA_SIZE 21
const init_option_t init_data[INIT_DATA_SIZE] =
    {
        {
            .data = 0, .size = 1, .buffer = {0x3c} // Boarder Waveform
        },
        {.data = 1, .size = 1, .buffer = {0x05}},
        {
            .data = 0, .size = 1, .buffer = {0x01} // Driver output control
        },
        {
            .data = 1,
            .size = 3,
            .buffer = {0x27, 0x01,
                       0x00} // Replace 0..1 with height - 1 (lsb first)
        },
        {
            .data = 0, .size = 1, .buffer = {0x11} // Data entry mode
        },
        {.data = 1, .size = 1, .buffer = {0x01}},
        {
            .data = 0, .size = 1, .buffer = {0x44} // RAM X address start/end.
        },
        {
            .data = 1,
            .size = 2,
            .buffer = {0x00, 0x0f} // 0 = start of 0x00, 1 = EPD_WIDTH/8-1 end
        },
        {
            .data = 0, .size = 1, .buffer = {0x45} // RAM Y address start/end.
        },
        {
            .data = 1,
            .size = 4,
            .buffer = {0x27, 0x01, 0x00, 0x00} // 0-1 = EPD_HEIGHT-1 LSB first
        },
        {
            .data = 0, .size = 1, .buffer = {0x21} // Display update control
        },
        {.data = 1, .size = 2, .buffer = {0x00, 0x80}},
        {
            .data = 0, .size = 1, .buffer = {0x18} // Set up temperature sensor
        },
        {
            .data = 1, .size = 1, .buffer = {0x80} // Internal thermometer
        },
        {
            .data = 0, .size = 1, .buffer = {0x4e} // Set RAM X address to 0
        },
        {.data = 1, .size = 1, .buffer = {0x00}},
        {
            .data = 0,
            .size = 1,
            .buffer = {0x4f} // Set RAM Y address to the end of height
        },
        {
            .data = 1,
            .size = 2,
            .buffer = {0x27, 0x01} // 0-1 = EPD_HEIGHT-1 LSB first
        },
        {
          .data = 0,
          .size = 1,
          .buffer = {0x22}
        },
                {
          .data = 1,
          .size = 1,
          .buffer = {0xb1}
        },
                {
          .data = 0,
          .size = 1,
          .buffer = {0x20}
        },
};

spi_transaction_t *
ssd1680_configure_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  uint8_t i = epaper->init_phase;

  if (i >= INIT_DATA_SIZE) {
    pin_write(epaper->dc, false);
    fsm_transition(&epaper->fsm, &ssd1680_power_off);
    return NULL;
  }
  
  pin_write(epaper->dc, init_data[i].data == 1);

  for (uint8_t j; j < init_data[i].size; j++) {
    epaper->cmd_buffer[j] = init_data[i].buffer[j];
  }

  spi->write_size = init_data[i].size;
  spi->buffer = &epaper->cmd_buffer[0];
  spi->callback = &ssd1680_configure_enter_spi_callback;

  epaper->init_phase++;

  return spi;
}

void ssd1680_configure_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  spi_transaction_t *t = ssd1680_configure_enter_spi_callback(&epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_configure= {
    .name = "CONFIGURE",
    .next_states = {&ssd1680_power_off, NULL },
                    .enter = ssd1680_configure_enter };

// POWER OFF

void ssd1680_power_off_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  if (epaper->pwr != PORTPIN_NONE) {
    pin_write(epaper->pwr, false);
  }

  fsm_transition(fsm, &ssd1680_idle);
}

const fs_t ssd1680_power_off = {.name = "POWER_ON",
                                .next_states = {&ssd1680_idle, NULL},
                                .enter = ssd1680_power_off_enter};