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

#define FILL_DATA_SIZE 9
const spi_queue_t fill_data[FILL_DATA_SIZE] = {
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_SET_RAM_X}},
    {
        .data = 1, .size = 2, .buffer = {0xaa, 0xbb} // 0 = Start, 1 = End
    },
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_SET_RAM_Y}},
    {
        .data = 1,
        .size = 4,
        .buffer = {0xaa, 0xaa, 0xbb, 0xbb} // 0-1 = Start, 2-3 = End
    },
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_SET_RAM_X_COUNTER}},
    {.data = 1, .size = 1, .buffer = {0xaa}},
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_SET_RAM_Y_COUNTER}},
    {.data = 1, .size = 2, .buffer = {0xaa, 0xaa}},
    {.data = 0, .size = 1, .buffer = {SSD1680_CMD_WRITE_RAM_BW}}};

spi_transaction_t *
ssd1680_operation_fill_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  uint16_t x1;
  uint16_t y1;
  uint16_t x2;
  uint16_t y2;

  switch (epaper->rotation) {
  case ROTATION_0:
    x1 = epaper->commited->data.fill.x1;
    x2 = epaper->commited->data.fill.x2;
    y1 = epaper->commited->data.fill.y1;
    y2 = epaper->commited->data.fill.y2;
    break;
  case ROTATION_90:
    x1 = epaper->commited->data.fill.y1;
    x2 = epaper->commited->data.fill.y2;
    y1 = epaper->height - epaper->commited->data.fill.x2 - 1;
    y2 = epaper->height - epaper->commited->data.fill.x1 - 1;
    break;
  case ROTATION_180:
    x1 = epaper->width - epaper->commited->data.fill.x2 - 1;
    x2 = epaper->width - epaper->commited->data.fill.x1 - 1;
    y1 = epaper->height - epaper->commited->data.fill.y2 - 1;
    y2 = epaper->height - epaper->commited->data.fill.y1 - 1;
    break;
  case ROTATION_270:
    x1 = epaper->width - epaper->commited->data.fill.y2;
    x2 = epaper->width - epaper->commited->data.fill.y1;
    y1 = epaper->commited->data.fill.x1;
    y2 = epaper->commited->data.fill.x2;
    break;
  }
  
  epaper->commited->mapped.width = ((x2 - x1)/8) + 1;
  epaper->commited->mapped.height = (y2 - y1) + 1;

  x1 = (x1 / 8);
  x2 = (x2 / 8);

  if (spi_queue_process(&fill_data, FILL_DATA_SIZE, epaper->dc,
                        &epaper->spi_transaction, &epaper->phase)) {
    switch (epaper->phase - 1) {
    case 1: // RAM X
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_270) {
        epaper->spi_transaction.buffer[0] = x1;
        epaper->spi_transaction.buffer[1] = x2;
      } else {
        epaper->spi_transaction.buffer[0] = x2;
        epaper->spi_transaction.buffer[1] = x1;
      }
      break;
    case 3: // RAM Y
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_90) {
        epaper->spi_transaction.buffer[0] = (uint8_t)(y1 & 0xff);
        epaper->spi_transaction.buffer[1] = (uint8_t)((y1 >> 8) & 0x01);
        epaper->spi_transaction.buffer[2] = (uint8_t)(y2 & 0xff);
        epaper->spi_transaction.buffer[3] = (uint8_t)((y2 >> 8) & 0x01);
      } else {
        epaper->spi_transaction.buffer[0] = (uint8_t)(y2 & 0xff);
        epaper->spi_transaction.buffer[1] = (uint8_t)((y2 >> 8) & 0x01);
        epaper->spi_transaction.buffer[2] = (uint8_t)(y1 & 0xff);
        epaper->spi_transaction.buffer[3] = (uint8_t)((y1 >> 8) & 0x01);
      }
      break;
    case 5: // RAM X Cursor
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_270) {
        epaper->spi_transaction.buffer[0] = x1;
      } else {
        epaper->spi_transaction.buffer[0] = x2;
      }
      break;
    case 7: // RAM Y Cursor
      if (epaper->rotation == ROTATION_0 || epaper->rotation == ROTATION_90) {
        epaper->spi_transaction.buffer[0] = (uint8_t)(y1 & 0xff);
        epaper->spi_transaction.buffer[1] = (uint8_t)((y1 >> 8) & 0x01);
      } else {
        epaper->spi_transaction.buffer[0] = (uint8_t)(y2 & 0xff);
        epaper->spi_transaction.buffer[1] = (uint8_t)((y2 >> 8) & 0x01);
      }
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
    switch (epaper->commited->colour) {
    case OPERATION_COLOUR_WHITE:
    case OPERATION_COLOUR_RED:
      epaper->cmd_buffer[i] = 0xff;
      break;
    case OPERATION_COLOUR_BLACK:
      epaper->cmd_buffer[i] = 0x00;
      break;
    }
  }

  pin_write(epaper->dc, true);
  epaper->spi_transaction.callback =
      &ssd1680_operation_fill_send_bw_enter_spi_callback;
  epaper->spi_transaction.write_size = 4;
  epaper->spi_transaction.write_repeats =
      ((epaper->commited->mapped.height * epaper->commited->mapped.width) / 4) - 1;

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
  fsm_transition(&epaper->fsm, &ssd1680_queue_return);
  return NULL;
}

void ssd1680_operation_fill_send_red_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  for (uint8_t i = 0; i < 4; i++) {
    switch (epaper->commited->colour) {
    case OPERATION_COLOUR_RED:
      epaper->cmd_buffer[i] = 0xff;
      break;
    case OPERATION_COLOUR_WHITE:
    case OPERATION_COLOUR_BLACK:
      epaper->cmd_buffer[i] = 0x00;
      break;
    }
  }

  pin_write(epaper->dc, true);
  epaper->spi_transaction.callback =
      &ssd1680_operation_fill_send_red_enter_spi_callback;
  epaper->spi_transaction.write_size = 4;
  epaper->spi_transaction.write_repeats =
      ((epaper->commited->mapped.height * epaper->commited->mapped.width) / 4) - 1;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_operation_fill_send_red = {
    .name = "FILL RED",
    .next_states = {&ssd1680_queue_return, NULL},
    .enter = ssd1680_operation_fill_send_red_enter};