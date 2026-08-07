#include "sys/cbus.h"
#include "sys/gpio.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

#include <stdbool.h>
#include <string.h>

#define CBUS_PACKET_MAX_SIZE 64U
#define CBUS_ENCODED_MAX_SIZE (CBUS_PACKET_MAX_SIZE + 1U)
#define CBUS_TX_QUEUE_SIZE 4U

typedef struct {
    uint8_t length;
    uint8_t data[CBUS_ENCODED_MAX_SIZE + 1U];
} CBUS_TxPacket;

typedef struct {
    UART_HandleTypeDef handle;

    volatile bool rxPending;
    volatile bool txPending;

    volatile bool rxSynced;
    volatile uint8_t rxLength;
    uint8_t rxBuffer[CBUS_ENCODED_MAX_SIZE];

    volatile uint8_t txHead;
    volatile uint8_t txTail;
    volatile uint8_t txCount;
    uint8_t txOffset;
    CBUS_TxPacket txQueue[CBUS_TX_QUEUE_SIZE];
} cbus_t;

static cbus_t cbus;

static uint8_t COBS_Encode(uint8_t length, const uint8_t *data, uint8_t *encoded) {
    uint8_t *start = encoded;
    uint8_t *codePointer = encoded++;
    uint8_t code = 1U;

    for (uint8_t index = 0U; index < length; index++) {
        if (data[index] == 0U) {
            *codePointer = code;
            codePointer = encoded++;
            code = 1U;
        } else {
            *encoded++ = data[index];
            code++;

            if (code == 0xFFU) {
                *codePointer = code;
                codePointer = encoded++;
                code = 1U;
            }
        }
    }

    *codePointer = code;
    return (uint8_t)(encoded - start);
}

static bool COBS_Decode(uint8_t length, const uint8_t *encoded, uint8_t *decoded, uint8_t *decodedLength) {
    uint8_t readIndex = 0U;
    uint8_t writeIndex = 0U;

    while (readIndex < length) {
        uint8_t code = encoded[readIndex++];

        if (code == 0U) {
            return false;
        }

        uint8_t copyLength = (uint8_t)(code - 1U);

        if ((readIndex + copyLength) > length || (writeIndex + copyLength) > CBUS_PACKET_MAX_SIZE) {
            return false;
        }

        memcpy(&decoded[writeIndex], &encoded[readIndex], copyLength);
        readIndex = (uint8_t)(readIndex + copyLength);
        writeIndex = (uint8_t)(writeIndex + copyLength);

        if (code != 0xFFU && readIndex < length) {
            if (writeIndex >= CBUS_PACKET_MAX_SIZE) {
                return false;
            }

            decoded[writeIndex++] = 0U;
        }
    }

    *decodedLength = writeIndex;
    return true;
}

static void CBUS_ProcessRxByte(uint8_t value, void (*packetHandler)(uint8_t length, void *data)) {
    if (!cbus.rxSynced) {
        cbus.rxSynced = value == 0U;
        return;
    }

    if (value == 0U) {
        if (cbus.rxLength > 0U && packetHandler != NULL) {
            uint8_t decodedLength;
            uint8_t decoded[CBUS_PACKET_MAX_SIZE];

            if (COBS_Decode(cbus.rxLength, cbus.rxBuffer, decoded, &decodedLength)) {
                packetHandler(decodedLength, decoded);
            }
        }

        cbus.rxLength = 0U;
        return;
    }

    if (cbus.rxLength >= CBUS_ENCODED_MAX_SIZE) {
        cbus.rxLength = 0U;
        cbus.rxSynced = false;
        return;
    }

    cbus.rxBuffer[cbus.rxLength++] = value;
}

static void CBUS_DrainRx(void (*packetHandler)(uint8_t length, void *data)) {
    while (__HAL_UART_GET_FLAG(&cbus.handle, UART_FLAG_RXFNE)) {
        uint8_t value = (uint8_t)(cbus.handle.Instance->RDR & 0xFFU);
        CBUS_ProcessRxByte(value, packetHandler);
    }

    cbus.rxPending = false;
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_RXFT);
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_IDLE);
}

static void CBUS_FillTx(void) {
    while (cbus.txCount > 0U && __HAL_UART_GET_FLAG(&cbus.handle, UART_FLAG_TXFNF)) {
        CBUS_TxPacket *packet = &cbus.txQueue[cbus.txTail];
        cbus.handle.Instance->TDR = packet->data[cbus.txOffset++];

        if (cbus.txOffset >= packet->length) {
            cbus.txOffset = 0U;
            cbus.txTail = (uint8_t)((cbus.txTail + 1U) % CBUS_TX_QUEUE_SIZE);
            cbus.txCount--;
        }
    }

    cbus.txPending = false;

    if (cbus.txCount > 0U) {
        __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_TXFNF);
    } else {
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_TXFNF);
    }
}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == UART5) {
        /* Peripheral clock enable */
        __HAL_RCC_UART5_CONFIG(RCC_UART5CLKSOURCE_PLL2Q);
        __HAL_RCC_UART5_CLK_ENABLE();

        /* UART5 GPIO Configuration
         * PB5     ------> UART5_RX
         * PB6     ------> UART5_TX
         */
        GPIO_InitStruct.Pin = CBUS_RX_Pin | CBUS_TX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF14_UART5;
        HAL_GPIO_Init(CBUS_RX_Port, &GPIO_InitStruct);
    }
}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART5) {
        __HAL_RCC_UART5_CLK_DISABLE();

        /* UART5 GPIO Configuration
         * PB5     ------> UART5_RX
         * PB6     ------> UART5_TX
         */
        HAL_GPIO_DeInit(CBUS_RX_Port, CBUS_RX_Pin | CBUS_TX_Pin);
    }
}

void CBUS_Init(void) {
    cbus.handle.Instance = UART5;
    cbus.handle.Init.BaudRate = 115200;
    cbus.handle.Init.WordLength = UART_WORDLENGTH_8B;
    cbus.handle.Init.StopBits = UART_STOPBITS_1;
    cbus.handle.Init.Parity = UART_PARITY_NONE;
    cbus.handle.Init.Mode = UART_MODE_TX_RX;
    cbus.handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    cbus.handle.Init.OverSampling = UART_OVERSAMPLING_16;
    cbus.handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    cbus.handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    cbus.handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&cbus.handle) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&cbus.handle, UART_TXFIFO_THRESHOLD_1_2) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&cbus.handle, UART_RXFIFO_THRESHOLD_1_2) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_EnableFifoMode(&cbus.handle) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(UART5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART5_IRQn);

    __HAL_UART_CLEAR_FLAG(&cbus.handle, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF |
                                        UART_CLEAR_IDLEF);
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_ERR);
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_RXFT);
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_IDLE);
    __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_TXFNF);
}

void CBUS_Service(void (*packetHandler)(uint8_t length, void *data)) {
    if (cbus.rxPending || __HAL_UART_GET_FLAG(&cbus.handle, UART_FLAG_RXFNE)) {
        CBUS_DrainRx(packetHandler);
    }

    if (cbus.txPending || (cbus.txCount > 0U && __HAL_UART_GET_FLAG(&cbus.handle, UART_FLAG_TXFNF))) {
        CBUS_FillTx();
    }
}

void CBUS_Queue(uint8_t length, void *data) {
    if (length > CBUS_PACKET_MAX_SIZE || data == NULL || cbus.txCount >= CBUS_TX_QUEUE_SIZE) {
        return;
    }

    CBUS_TxPacket *packet = &cbus.txQueue[cbus.txHead];
    packet->length = COBS_Encode(length, data, packet->data);
    packet->data[packet->length++] = 0U;

    cbus.txHead = (uint8_t)((cbus.txHead + 1U) % CBUS_TX_QUEUE_SIZE);
    cbus.txCount++;

    cbus.txPending = true;
    __HAL_UART_ENABLE_IT(&cbus.handle, UART_IT_TXFNF);
}

void UART5_IRQHandler(void) {
    uint32_t flags = cbus.handle.Instance->ISR;

    if ((flags & (UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE)) != 0U) {
        __HAL_UART_CLEAR_FLAG(&cbus.handle, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
        cbus.rxSynced = false;
        cbus.rxLength = 0U;
        cbus.rxPending = true;
    }

    if ((flags & UART_FLAG_IDLE) != 0U) {
        __HAL_UART_CLEAR_FLAG(&cbus.handle, UART_CLEAR_IDLEF);
        cbus.rxPending = true;
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_RXFT);
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_IDLE);
    }

    if ((flags & UART_FLAG_RXFT) != 0U) {
        cbus.rxPending = true;
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_RXFT);
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_IDLE);
    }

    if ((flags & UART_FLAG_TXFNF) != 0U && cbus.txCount > 0U) {
        cbus.txPending = true;
        __HAL_UART_DISABLE_IT(&cbus.handle, UART_IT_TXFNF);
    }
}
