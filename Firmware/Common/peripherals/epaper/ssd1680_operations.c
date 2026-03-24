#include <xc.h>
#include "../../utils/fsm.h"
#include "../../hal/spi.h"
#include "epaper.h"
#include "ssd1680.h"
#include "ssd1680_operations.h"

void ssd1680_operation_fill_enter(fsm_t *fsm) {
  epaper_t *epaper = (epaper_t *)fsm->ctx;


}

const fs_t ssd1680_operation_fill = {
    .name = "QUEUE",
    .next_states = {&ssd1680_queue_return, NULL},
    .enter = ssd1680_operation_fill_enter};