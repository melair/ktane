#ifndef CSP_H
#define CSP_H

#include <stdint.h>
#include "../../hal/pin.h"

#define CSP_PAYLOAD_MAX 16
#define CSP_OBJECT_COUNT 4 

#define CSP_NO_OBJECT 0xff

typedef struct {
    uint8_t address;

    uint8_t buffer[CSP_PAYLOAD_MAX * CSP_OBJECT_COUNT];

    uint8_t buffer_busy;
    uint8_t buffer_tx;

} csp_t;

void csp_init(csp_t *csp, pin_t rx, pin_t tx, pin_t de, uint8_t uart);
void csp_service(csp_t *csp);
void csp_interrupt_tx(csp_t *csp);
void csp_interrupt_rx(csp_t *csp);
void csp_set_addr(csp_t *csp, uint8_t addr);
uint8_t csp_get_rx_obj(csp_t *csp);
uint8_t csp_get_tx_obj(csp_t *csp);
void csp_send_tx_obj(csp_t *csp, uint8_t obj);
uint8_t *csp_get_object(csp_t *csp, uint8_t obj);

#endif