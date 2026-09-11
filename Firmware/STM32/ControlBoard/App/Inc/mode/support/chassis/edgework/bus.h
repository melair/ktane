#ifndef EDGEWORK_BUS_H
#define EDGEWORK_BUS_H

#include <stdbool.h>

#include "cobs/cobs.h"
#include "edgework/bus/protocol.h"
#include "stm32h5xx_hal.h"
#include "uart/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUS_TX_FRAME_COUNT 4U

typedef struct {
    UART_State uart;
    UART_HandleTypeDef uart_handle;
    COBS_State cobs;
    uint8_t uart_tx_buffer[BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];
} Bus_Data;

bool Bus_Init(void);

void Bus_Service(void);

bool Bus_Send(const EdgeworkBus_Packet *packet, size_t length);

#ifdef __cplusplus
}
#endif

#endif // EDGEWORK_BUS_H
