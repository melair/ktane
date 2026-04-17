#ifndef CSP_H
#define CSP_H

#include <stdint.h>
#include "../../hal/pin.h"

#define CSP_PAYLOAD_MAX 16
#define CSP_OBJECT_COUNT 4 

#define CSP_NO_OBJECT 0xff

typedef struct {
    uint8_t uart;
    uint8_t config;
    uint8_t address;

    unsigned in_sync :1;

    uint8_t buffer[CSP_PAYLOAD_MAX * CSP_OBJECT_COUNT];
} csp_t;

#define CFG_CSP_HALF_DUPLEX 0b00000000
#define CFG_CSP_FULL_DUPLEX 0b00000001

#define CSP_ADDR_COORDINATOR 0x00
#define CSP_ADDR_UNKNOWN 0xfe
#define CSP_ADDR_GENERAL_CALL 0xff

void csp_init(csp_t *csp, pin_t rx, pin_t tx, pin_t de, uint8_t uart, uint8_t cfg);
void csp_service(csp_t *csp);
void csp_interrupt_tx(csp_t *csp);
void csp_interrupt_rx(csp_t *csp);
void csp_set_addr(csp_t *csp, uint8_t addr);
uint8_t csp_get_rx_obj(csp_t *csp);
uint8_t csp_get_tx_obj(csp_t *csp);
void csp_send_tx_obj(csp_t *csp, uint8_t obj);
uint8_t *csp_get_object(csp_t *csp, uint8_t obj);

#endif