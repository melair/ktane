#include <xc.h>
#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../hal/spi_queue.h"
#include "vfd.h"
#include "../../utils/fsm.h"

const fs_t vfd_state_init_reset;
const fs_t vfd_state_init_wait;
const fs_t vfd_state_init_spi;
const fs_t vfd_state_idle;

static void vfd_state_init_reset_enter(fsm_t *fsm) {
    vfd_t *vfd = (vfd_t *) fsm->ctx;

    pin_config(vfd->en, OUTPUT, 0);
    pin_write(vfd->en, true);
    pin_config(vfd->reset, OUTPUT, 0);
    pin_write(vfd->reset, false);
    pin_config(vfd->cs, OUTPUT, 0);
    pin_write(vfd->cs, true);

    fsm_transition_in(fsm, &vfd_state_init_wait, 10);
}

static void vfd_state_init_reset_exit(fsm_t *fsm) {
    vfd_t *vfd = (vfd_t *) fsm->ctx;

    pin_write(vfd->reset, true);
}

const fs_t vfd_state_init_reset = { .enter = &vfd_state_init_reset_enter, .exit = &vfd_state_init_reset_exit, .next_states = { &vfd_state_init_wait, NULL} };

static void vfd_state_init_wait_enter(fsm_t *fsm) {
    fsm_transition_in(fsm, &vfd_state_init_spi, 10);
}

const fs_t vfd_state_init_wait = { .enter = &vfd_state_init_wait_enter, .next_states = {  &vfd_state_init_spi, NULL } };

#define INIT_DATA_SIZE 4

static const spi_queue_element_t init_data[INIT_DATA_SIZE] = {
    {.cs_bounce = 1, BUFFER(0xe0)}, // Set Timing
    {.cs_bounce = 0, BUFFER(0x07)}, // 8-MD-06INK
    {.cs_bounce = 1, BUFFER(0xe4)}, // Set Brightness
    {.cs_bounce = 0, BUFFER(0xff)}, // Full
};

static spi_transaction_t *vfd_state_init_spi_callback(spi_transaction_t *spi) {
  vfd_t *vfd = (vfd_t *)spi->callback_data;

  if (spi_queue_process(&vfd->spi_queue, &vfd->spi_transaction)) {
    return spi;
  } else {
    fsm_transition(&vfd->fsm, &vfd_state_idle);
    return NULL;
  }
}

static void vfd_state_init_spi_enter(fsm_t *fsm) {
  vfd_t *vfd = (vfd_t *) fsm->ctx;
  vfd->spi_transaction.callback = &vfd_state_init_spi_callback;

  spi_queue_init(&vfd->spi_queue, &init_data, INIT_DATA_SIZE, PORTPIN_NONE);

  spi_transaction_t *t = vfd_state_init_spi_callback(&vfd->spi_transaction);
  spi_queue(t);
}

const fs_t vfd_state_init_spi = { .enter = &vfd_state_init_spi_enter, .next_states = { &vfd_state_idle, NULL }};

static void vfd_state_idle_service(fsm_t *fsm) {

}

const fs_t vfd_state_idle = { .service = &vfd_state_idle_service, .next_states = { NULL }};

#define DISPLAY_DATA_SIZE 10

static const spi_queue_element_t display_data[DISPLAY_DATA_SIZE] = {
    {.cs_bounce = 1, BUFFER(0x20)}, // Set CDRAM pos to 0x00. (0x20 + x)
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 0, BUFFER('?')}, 
    {.cs_bounce = 1, BUFFER(0xe8)}, // Transfer data to display (show)
};

void vfd_init(vfd_t *vfd, pin_t en, pin_t reset, pin_t cs) {
    vfd->en = en;
    vfd->reset = reset;
    vfd->cs = cs;

    vfd->spi_transaction.cs_pin = vfd->cs;
    vfd->spi_transaction.cs_bounce = true;
    vfd->spi_transaction.cs_wait_ms = 0;
    vfd->spi_transaction.baud = SPI_BAUD_125K;
    vfd->spi_transaction.bits = 8;
    vfd->spi_transaction.cke = 1;
    vfd->spi_transaction.lsb_first = 0;
    vfd->spi_transaction.operation = SPI_OPERATION_WRITE;
    vfd->spi_transaction.write_repeats = 0;
    vfd->spi_transaction.write_size = 0;
    vfd->spi_transaction.read_size = 0;
    vfd->spi_transaction.callback_data = vfd;

    fsm_init(&vfd->fsm, &vfd_state_init_reset, vfd);

    for (uint8_t i = 0; i < VFD_SIZE; i++) {
        vfd_set(vfd, i, ' ');
    }

    vfd_update(vfd);
}   

void vfd_service(vfd_t *vfd) {
    fsm_service(&vfd->fsm);
}

void vfd_set(vfd_t *vfd, uint8_t i, uint8_t chr) {
    vfd->edit_buffer[i] = chr;
}

void vfd_update(vfd_t *vfd) {
    vfd->update_request = true;
}