#ifndef EPAPER_H
#define EPAPER_H

#include <stdbool.h>

#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../hal/spi_queue.h"
#include "../../utils/fsm.h"

#define EPAPER_TYPE_SSD1680 0

typedef struct epaper_command_t epaper_command_t;
typedef struct epaper_t epaper_t;

#define OPERATION_FILL 0
#define OPERATION_COPY_FROM_FLASH 1

#define OPERATION_COLOUR_WHITE 0
#define OPERATION_COLOUR_BLACK 1
#define OPERATION_COLOUR_RED 2

struct epaper_command_t {
  uint8_t operation;
  union {
    struct {
      uint8_t colour;
    } fill;
    struct {
      const uint8_t *addr;
      uint8_t fg_colour;
      uint8_t bg_colour;
    } copy_from_flash;
  } operation_data;

  struct {
    uint16_t x;
    uint16_t y;

    uint16_t width;
    uint16_t height;
  } canvas;

  struct {
    uint16_t x1;
    uint16_t x2;
    uint16_t y1;
    uint16_t y2;
    uint16_t bytes;
  } _mapped;

  epaper_command_t *next;
};

#define ROTATION_0 0
#define ROTATION_90 1
#define ROTATION_180 2
#define ROTATION_270 3

struct epaper_t {
  pin_t cs;
  pin_t pwr;
  pin_t dc;
  pin_t reset;
  pin_t busy;

  uint8_t type;
  uint16_t width;
  uint16_t height;
  uint8_t rotation;
  struct {
    unsigned red :1;
  } colours;

  fsm_t fsm;

  epaper_command_t *queue_head;
  epaper_command_t *queue_tail;

  epaper_command_t *commited;
  bool partial;

  spi_queue_t spi_queue;
  spi_transaction_t spi_transaction;
  uint8_t cmd_buffer[4];
  uint8_t progress;
};

void epaper_init(epaper_t *epaper);
void epaper_service(epaper_t *epaper);
void epaper_queue(epaper_t *epaper, epaper_command_t *cmd);
void epaper_refresh(epaper_t *epape, bool partial);

#endif