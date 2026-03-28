#include "epaper.h"
#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "ssd1680.h"
#include <xc.h>

void epaper_init(epaper_t *epaper) {
  epaper->fsm.ctx = epaper;
  epaper->fsm.initial = &ssd1680_idle;

  epaper->queue_head = NULL;
  epaper->queue_tail = NULL;
  epaper->commited = NULL;

  fsm_init(&epaper->fsm);

  epaper->spi_transaction.cs_pin = epaper->cs;
  epaper->spi_transaction.cs_bounce = true;
  epaper->spi_transaction.cs_wait_ms = 0;
  epaper->spi_transaction.baud = SPI_BAUD_2000K;
  epaper->spi_transaction.bits = 8;
  epaper->spi_transaction.cke = 1;
  epaper->spi_transaction.lsb_first = 0;
  epaper->spi_transaction.operation = SPI_OPERATION_WRITE;
  epaper->spi_transaction.write_repeats = 0;
  epaper->spi_transaction.write_size = 0;
  epaper->spi_transaction.read_size = 0;
  epaper->spi_transaction.callback_data = epaper;
}

void epaper_service(epaper_t *epaper) { fsm_service(&epaper->fsm); }

void epaper_queue(epaper_t *epaper, epaper_command_t *cmd) {
  cmd->next = NULL;

  if (epaper->queue_head == NULL) {
    epaper->queue_head = cmd;
  } else {
    epaper->queue_tail->next = cmd;
  }

  epaper->queue_tail = cmd;
}

void epaper_refresh(epaper_t *epaper, bool partial) {
  if (epaper->commited != NULL || epaper->queue_head == NULL) {
    return;
  }

  epaper->partial = partial;
  epaper->commited = epaper->queue_head;
  epaper->queue_head = NULL;
  epaper->queue_tail = NULL;
}