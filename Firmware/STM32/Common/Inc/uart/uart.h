#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_RX_BUFFER_SIZE 64U

typedef struct UART_Hardware UART_Hardware;

typedef void (*UART_RxHandler)(const uint8_t *data, size_t length);

typedef struct {
    void *platform_handle;

    volatile bool rx_pending;
    volatile bool tx_pending;

    uint8_t rx_buffer[UART_RX_BUFFER_SIZE];

    uint8_t *tx_buffer;
    size_t tx_capacity;
    volatile size_t tx_head;
    volatile size_t tx_tail;
    volatile size_t tx_count;
} UART_State;

bool UART_Init(UART_State *uart, const UART_Hardware *hardware,
               uint8_t *tx_buffer, size_t tx_capacity);

void UART_Service(UART_State *uart, UART_RxHandler rx_handler);

bool UART_Queue(UART_State *uart, const uint8_t *data, size_t length);

void UART_IRQHandler(UART_State *uart);

#ifdef __cplusplus
}
#endif

#endif //UART_H
