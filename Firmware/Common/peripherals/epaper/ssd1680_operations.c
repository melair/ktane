#include "ssd1680_operations.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "epaper.h"
#include "spi_queue.h"
#include "ssd1680.h"
#include <xc.h>

extern const fs_t ssd1680_operation_fill_send_bw;
extern const fs_t ssd1680_operation_fill_send_red_cmd;
extern const fs_t ssd1680_operation_fill_send_red;

#define FILL_DATA_SIZE 5
const spi_queue_t fill_data[FILL_DATA_SIZE] = {
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_SET_RAM_X_COUNTER} // Set RAM X address to 0
    },
    {.data = 1, .size = 1, .buffer = {0x00}},
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_SET_RAM_Y_COUNTER} // Set RAM Y address to the
                                                  // end of height
    },
    {
        .data = 1,
        .size = 2,
        .buffer = {0xff, 0xff} // 0-1 = EPD_HEIGHT-1 LSB first
    },
    {
        .data = 0,
        .size = 1,
        .buffer = {SSD1680_CMD_WRITE_RAM_BW} // Set black/white data.
    }};

spi_transaction_t *
ssd1680_operation_fill_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  if (spi_queue_process(&fill_data, FILL_DATA_SIZE, epaper->dc,
                        &epaper->spi_transaction, &epaper->phase)) {
    switch (epaper->phase - 1) {
    case 3:
      uint16_t height = epaper->height - 1;
      epaper->spi_transaction.buffer[0] = (uint8_t)(height & 0xff);
      epaper->spi_transaction.buffer[1] = (uint8_t)((height >> 8) & 0x01);
      break;
    }

    return spi;
  } else {
    fsm_transition(&epaper->fsm, &ssd1680_operation_fill_send_bw);
    return NULL;
  }
}

void ssd1680_operation_fill_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback = &ssd1680_operation_fill_enter_spi_callback;

  spi_transaction_t *t =
      ssd1680_operation_fill_enter_spi_callback(&epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_operation_fill = {
    .name = "FILL",
    .next_states = {&ssd1680_operation_fill_send_bw, NULL},
    .enter = ssd1680_operation_fill_enter};

spi_transaction_t *
ssd1680_operation_fill_send_bw_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  pin_write(epaper->dc, false);
  fsm_transition(&epaper->fsm, &ssd1680_operation_fill_send_red_cmd);
  return NULL;
}

void ssd1680_operation_fill_send_bw_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  for (uint8_t i = 0; i < 4; i++) {
    switch (epaper->commited->operation) {
    case OPERATION_FILL_WHITE:
    case OPERATION_FILL_RED:
      epaper->cmd_buffer[i] = 0xff;
      break;
    case OPERATION_FILL_BLACK:
      epaper->cmd_buffer[i] = 0x00;
      break;
    }
  }

  pin_write(epaper->dc, true);
  epaper->spi_transaction.callback =
      &ssd1680_operation_fill_send_bw_enter_spi_callback;
  epaper->spi_transaction.write_size = 4;
  epaper->spi_transaction.write_repeats =
      (((epaper->height / 8) * (epaper->width)) / 4) - 1;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_operation_fill_send_bw = {
    .name = "FILL BW",
    .next_states = {&ssd1680_operation_fill_send_red_cmd, NULL},
    .enter = ssd1680_operation_fill_send_bw_enter};

spi_transaction_t *
ssd1680_operation_fill_send_red_cmd_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  fsm_transition(&epaper->fsm, &ssd1680_operation_fill_send_red);
  return NULL;
}

void ssd1680_operation_fill_send_red_cmd_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  epaper->spi_transaction.callback =
      &ssd1680_operation_fill_send_red_cmd_callback;
  epaper->spi_transaction.write_size = 1;
  epaper->spi_transaction.write_repeats = 0;
  epaper->spi_transaction.buffer[0] = SSD1680_CMD_WRITE_RAM_RED;

  spi_queue(&epaper->spi_transaction);
}
const fs_t ssd1680_operation_fill_send_red_cmd = {
    .name = "FILL RED CMD",
    .next_states = {&ssd1680_operation_fill_send_red, NULL},
    .enter = ssd1680_operation_fill_send_red_cmd_enter};

extern const fs_t temp;


spi_transaction_t *
ssd1680_operation_fill_send_red_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  pin_write(epaper->dc, false);
  fsm_transition(&epaper->fsm, &temp);
  return NULL;
}

void ssd1680_operation_fill_send_red_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  for (uint8_t i = 0; i < 4; i++) {
    switch (epaper->commited->operation) {
    case OPERATION_FILL_RED:
      epaper->cmd_buffer[i] = 0xff;
      break;
    case OPERATION_FILL_WHITE:
    case OPERATION_FILL_BLACK:
      epaper->cmd_buffer[i] = 0x00;
      break;
    }
  }
  epaper->cmd_buffer[0] = 0x80;
  epaper->cmd_buffer[1] = 0x80;
  epaper->cmd_buffer[2] = 0x00;
  epaper->cmd_buffer[3] = 0x00;
  
  pin_write(epaper->dc, true);
  epaper->spi_transaction.callback =
      &ssd1680_operation_fill_send_red_enter_spi_callback;
  epaper->spi_transaction.write_size = 4;
  epaper->spi_transaction.write_repeats =
      ((((epaper->height / 8) * (epaper->width)) / 4)/4) - 1;

  spi_queue(&epaper->spi_transaction);
}


const fs_t ssd1680_operation_fill_send_red = {
    .name = "FILL RED",
    .next_states = {&temp, NULL},
    .enter = ssd1680_operation_fill_send_red_enter};


spi_transaction_t *
temp_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  pin_write(epaper->dc, false);
  fsm_transition(&epaper->fsm, &ssd1680_queue_return);
  return NULL;
}

void temp_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  for (uint8_t i = 0; i < 4; i++) {
    switch (epaper->commited->operation) {
    case OPERATION_FILL_RED:
    case OPERATION_FILL_WHITE:
    case OPERATION_FILL_BLACK:
      epaper->cmd_buffer[i] = 0x00;
      break;
    }
  }

  pin_write(epaper->dc, true);
  epaper->spi_transaction.callback =
      &temp_callback;
  epaper->spi_transaction.write_size = 4;
  epaper->spi_transaction.write_repeats =
      (((((epaper->height / 8) * (epaper->width)) / 4)/4)*3) - 1;

  spi_queue(&epaper->spi_transaction);
}


const fs_t temp = {
    .name = "FILL TEMP",
    .next_states = {&ssd1680_queue_return, NULL},
    .enter = temp_enter};
