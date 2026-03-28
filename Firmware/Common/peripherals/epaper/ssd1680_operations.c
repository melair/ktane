#include "ssd1680_operations.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "epaper.h"
#include "spi_queue.h"
#include "ssd1680.h"
#include <stdint.h>
#include <xc.h>

extern const fs_t ssd1680_operation_fill_send_bw;
extern const fs_t ssd1680_operation_fill_send_red_cmd;
extern const fs_t ssd1680_operation_fill_send_red;

extern const fs_t ssd1680_operation_copy_from_flash_send_bw;
extern const fs_t ssd1680_operation_copy_from_flash_red_cmd;
extern const fs_t ssd1680_operation_copy_from_flash_send_red;

spi_transaction_t *
ssd1680_operation_fill_enter_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  fsm_transition(&epaper->fsm, &ssd1680_operation_fill_send_bw);
  return NULL;
}

void ssd1680_operation_fill_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  epaper->spi_transaction.callback = &ssd1680_operation_fill_enter_spi_callback;
  epaper->spi_transaction.write_size = 1;
  epaper->spi_transaction.write_repeats = 0;
  epaper->spi_transaction.buffer[0] = SSD1680_CMD_WRITE_RAM_BW;

  spi_queue(&epaper->spi_transaction);
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
    switch (epaper->commited->operation_data.fill.colour) {
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
      ((epaper->commited->_mapped.height * epaper->commited->_mapped.width) /
       4) -
      1;

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
    switch (epaper->commited->operation_data.fill.colour) {
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
      ((epaper->commited->_mapped.height * epaper->commited->_mapped.width) /
       4) -
      1;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_operation_fill_send_red = {
    .name = "FILL RED",
    .next_states = {&ssd1680_queue_return, NULL},
    .enter = ssd1680_operation_fill_send_red_enter};

spi_transaction_t *
ssd1680_operation_copy_from_flash_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  fsm_transition(&epaper->fsm, &ssd1680_operation_copy_from_flash_send_bw);
  return NULL;
}

void ssd1680_operation_copy_from_flash_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  epaper->spi_transaction.callback =
      &ssd1680_operation_copy_from_flash_spi_callback;
  epaper->spi_transaction.write_size = 1;
  epaper->spi_transaction.write_repeats = 0;
  epaper->spi_transaction.buffer[0] = SSD1680_CMD_WRITE_RAM_BW;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_operation_copy_from_flash = {
    .name = "CFF BW CMD",
    .next_states = {&ssd1680_operation_copy_from_flash_send_bw, NULL},
    .enter = ssd1680_operation_copy_from_flash_enter};

spi_transaction_t *
ssd1680_operation_copy_from_flash_send_bw_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  if (epaper->phase >= 140) {
    fsm_transition(&epaper->fsm, &ssd1680_operation_copy_from_flash_red_cmd);
    pin_write(epaper->dc, false);
    return NULL;
  }

  // Use data directly.
  spi->callback = &ssd1680_operation_copy_from_flash_send_bw_spi_callback;
  spi->write_size = 4;
  spi->write_repeats = 0;

  switch (epaper->commited->operation_data.copy_from_flash.fg_colour) {
  case OPERATION_COLOUR_WHITE:
    spi->buffer[0] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[1] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[2] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[3] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    break;
  case OPERATION_COLOUR_BLACK:
    spi->buffer[0] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[1] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[2] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[3] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    break;
  case OPERATION_COLOUR_RED:
    if (epaper->commited->operation_data.copy_from_flash.bg_colour ==
        OPERATION_COLOUR_WHITE) {
      spi->buffer[0] = epaper->commited->operation_data.copy_from_flash
                           .addr[epaper->phase++];
      spi->buffer[1] = epaper->commited->operation_data.copy_from_flash
                           .addr[epaper->phase++];
      spi->buffer[2] = epaper->commited->operation_data.copy_from_flash
                           .addr[epaper->phase++];
      spi->buffer[3] = epaper->commited->operation_data.copy_from_flash
                           .addr[epaper->phase++];
    } else {
      spi->buffer[0] = ~epaper->commited->operation_data.copy_from_flash
                            .addr[epaper->phase++];
      spi->buffer[1] = ~epaper->commited->operation_data.copy_from_flash
                            .addr[epaper->phase++];
      spi->buffer[2] = ~epaper->commited->operation_data.copy_from_flash
                            .addr[epaper->phase++];
      spi->buffer[3] = ~epaper->commited->operation_data.copy_from_flash
                            .addr[epaper->phase++];
    }

    break;
  }

  return spi;
}

void ssd1680_operation_copy_from_flash_send_bw_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback =
      &ssd1680_operation_copy_from_flash_send_bw_spi_callback;
  pin_write(epaper->dc, true);

  spi_transaction_t *t = ssd1680_operation_copy_from_flash_send_bw_spi_callback(
      &epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_operation_copy_from_flash_send_bw = {
    .name = "CFF BW CMD",
    .next_states = {&ssd1680_operation_copy_from_flash_red_cmd, NULL},
    .enter = ssd1680_operation_copy_from_flash_send_bw_enter};

spi_transaction_t *
ssd1680_operation_copy_from_flash_red_cmd_spi_callback(spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;
  fsm_transition(&epaper->fsm, &ssd1680_operation_copy_from_flash_send_red);
  return NULL;
}

void ssd1680_operation_copy_from_flash_red_cmd_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;

  epaper->spi_transaction.callback =
      &ssd1680_operation_copy_from_flash_red_cmd_spi_callback;
  epaper->spi_transaction.write_size = 1;
  epaper->spi_transaction.write_repeats = 0;
  epaper->spi_transaction.buffer[0] = SSD1680_CMD_WRITE_RAM_RED;

  spi_queue(&epaper->spi_transaction);
}

const fs_t ssd1680_operation_copy_from_flash_red_cmd = {
    .name = "CFF RED CMD",
    .next_states = {&ssd1680_operation_copy_from_flash_send_red, NULL},
    .enter = ssd1680_operation_copy_from_flash_red_cmd_enter};

spi_transaction_t *ssd1680_operation_copy_from_flash_send_red_spi_callback(
    spi_transaction_t *spi) {
  epaper_t *epaper = (epaper_t *)spi->callback_data;

  if (epaper->phase >= 140) {
    fsm_transition(&epaper->fsm, &ssd1680_queue_return);
    pin_write(epaper->dc, false);
    return NULL;
  }

  // Use data directly.
  spi->callback = &ssd1680_operation_copy_from_flash_send_red_spi_callback;
  spi->write_size = 4;
  spi->write_repeats = 0;

  if (epaper->commited->operation_data.copy_from_flash.bg_colour ==
      OPERATION_COLOUR_RED) {
    spi->buffer[0] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[1] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[2] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[3] =
        ~epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
  } else if (epaper->commited->operation_data.copy_from_flash.fg_colour ==
             OPERATION_COLOUR_RED) {
    spi->buffer[0] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[1] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[2] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
    spi->buffer[3] =
        epaper->commited->operation_data.copy_from_flash.addr[epaper->phase++];
  } else {
    spi->buffer[0] = 0x00;
    spi->buffer[1] = 0x00;
    spi->buffer[2] = 0x00;
    spi->buffer[3] = 0x00;
    epaper->phase += 4;
  }

  return spi;
}

void ssd1680_operation_copy_from_flash_send_red_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;
  epaper->phase = 0;
  epaper->spi_transaction.callback =
      &ssd1680_operation_copy_from_flash_send_red_spi_callback;
  pin_write(epaper->dc, true);

  spi_transaction_t *t =
      ssd1680_operation_copy_from_flash_send_red_spi_callback(
          &epaper->spi_transaction);
  spi_queue(t);
}

const fs_t ssd1680_operation_copy_from_flash_send_red = {
    .name = "CFF RED",
    .next_states = {&ssd1680_queue_return, NULL},
    .enter = ssd1680_operation_copy_from_flash_send_red_enter};