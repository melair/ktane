#ifndef CSP_H
#define CSP_H

#include <stdint.h>
#include "../../hal/pin.h"

#define CSP_PAYLOAD_MAX 16
#define CSP_OBJECT_COUNT 4 
#define CSP_INACTIVE_OBJECT CSP_OBJECT_COUNT

#define CSP_NO_OBJECT 0xff

typedef struct csp_t csp_t;

struct csp_t {
    uint8_t uart;
    uint8_t config;
    uint8_t address;

    struct {
        unsigned in_sync: 1;
        unsigned last :4;
        unsigned active :4;
        unsigned pos;
    } rx;

    struct {
        unsigned active :4;
        unsigned pos;
    } tx;

    struct {
        uint8_t rx_overflow;
        uint8_t rx_giant;
        uint8_t rx_runt;
    } errors;

    uint8_t buffer[CSP_PAYLOAD_MAX * CSP_OBJECT_COUNT];
    uint8_t buffer_size[CSP_OBJECT_COUNT];
    uint8_t buffer_used;   // Unused = 0, Used = 1
    uint8_t buffer_type;   // RX = 0, TX = 1
    uint8_t buffer_ready;  // Not = 0, Ready for TX = 1

    void (*callback)(csp_t *csp, uint8_t *ptr, uint8_t len);
};

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
void csp_tx(csp_t *csp, uint8_t *ptr, uint8_t len);

#endif