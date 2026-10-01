#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_RX_BUFFER_SIZE 64U
#define UART_RX_RING_DEFAULT_CAPACITY 64U

typedef struct UART_Hardware UART_Hardware;

typedef void (*UART_RxHandler)(const uint8_t *data, size_t length);

typedef enum {
    UART_RX_FIFO = 0,
    UART_RX_BUFFERED,
    UART_RX_BUFFERED_ERROR,
} UART_RxState;

typedef struct {
    void *platform_handle;

    volatile bool rx_pending;
    volatile bool tx_pending;

    uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
    volatile UART_RxState rx_state;
    void (*rx_error_handler)(void);
    uint8_t *rx_ring;
    size_t rx_capacity;
    volatile size_t rx_head;
    volatile size_t rx_tail;
    volatile size_t rx_count;

    uint8_t *tx_buffer;
    size_t tx_capacity;
    volatile size_t tx_head;
    volatile size_t tx_tail;
    volatile size_t tx_count;
} UART_State;

/**
 * Buffers remain owned by the caller and must outlive the UART.
 * Non-FIFO instances require a receive ring with positive capacity; FIFO
 * instances may pass NULL, 0. Ring capacity need not be a power of two.
 */
bool UART_Init(UART_State *uart, const UART_Hardware *hardware,
               uint8_t *tx_buffer, size_t tx_capacity,
               uint8_t *rx_ring, size_t rx_capacity);

void UART_Service(UART_State *uart, UART_RxHandler rx_handler);

bool UART_Queue(UART_State *uart, const uint8_t *data, size_t length);

void UART_IRQHandler(UART_State *uart);

#ifdef __cplusplus
}
#endif

#endif //UART_H
