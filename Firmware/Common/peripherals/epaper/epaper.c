#include <xc.h>
#include "epaper.h"
#include "ssd1680.h"
#include "../../hal/spi.h"
#include "../../utils/fsm.h"
#include "../../hal/pin.h"

void epaper_init(epaper_t *epaper) {
    epaper->fsm.ctx = epaper;
    epaper->fsm.initial = &ssd1680_idle;

    epaper->queue_head = NULL;
    epaper->queue_tail = NULL;
    epaper->commited = NULL;

    fsm_init(&epaper->fsm);
}

void epaper_service(epaper_t *epaper) {
    fsm_service(&epaper->fsm);
}

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