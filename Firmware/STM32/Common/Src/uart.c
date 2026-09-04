#include "uart.h"
#include "uart_platform.h"

#include <string.h>

static UART_HandleTypeDef *uart_handle(const UART_State *uart) {
    return uart->platform_handle;
}

static bool hardware_valid(const UART_Hardware *hardware) {
    return (hardware != NULL) &&
           (hardware->uart_instance != NULL) &&
           (hardware->uart_handle != NULL) &&
           (hardware->rx.port != NULL) &&
           (hardware->tx.port != NULL) &&
           (hardware->configure_clock != NULL) &&
           (hardware->enable_peripheral_clock != NULL);
}

static void gpio_configure(const GPIO_PinDef *pin, uint32_t alternate) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(pin->port, &gpio_init);
}

static bool configure_rx_tx_swap(UART_HandleTypeDef *huart, bool swap_rx_tx) {
#if defined(UART_ADVFEATURE_SWAP_INIT)
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
    huart->AdvancedInit.Swap = swap_rx_tx ? UART_ADVFEATURE_SWAP_ENABLE : UART_ADVFEATURE_SWAP_DISABLE;
    return true;
#else
    return false;
#endif
}

static bool hardware_init(UART_State *uart, const UART_Hardware *hardware) {
    if (!hardware_valid(hardware)) {
        return false;
    }

    uart->platform_handle = hardware->uart_handle;
    UART_HandleTypeDef *huart = uart_handle(uart);
    memset(huart, 0, sizeof(*huart));

    if (!hardware->configure_clock()) {
        uart->platform_handle = NULL;
        return false;
    }

    hardware->enable_peripheral_clock();
    gpio_configure(&hardware->rx, hardware->rx_alternate);
    gpio_configure(&hardware->tx, hardware->tx_alternate);
    if (hardware->driver_enable.port != NULL) {
        gpio_configure(&hardware->driver_enable, hardware->driver_enable_alternate);
    }

    huart->Instance = hardware->uart_instance;
    huart->Init.BaudRate = hardware->baud_rate;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    if (!configure_rx_tx_swap(huart, hardware->swap_rx_tx)) {
        uart->platform_handle = NULL;
        return false;
    }

    const HAL_StatusTypeDef init_status = hardware->driver_enable.port == NULL
                                          ? HAL_UART_Init(huart)
                                          : HAL_RS485Ex_Init(huart,
                                                             hardware->driver_enable_active_low
                                                                 ? UART_DE_POLARITY_LOW
                                                                 : UART_DE_POLARITY_HIGH,
                                                             0U, 0U);

    if ((init_status != HAL_OK) ||
        (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_2) != HAL_OK) ||
        (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_2) != HAL_OK) ||
        (HAL_UARTEx_EnableFifoMode(huart) != HAL_OK)) {
        uart->platform_handle = NULL;
        return false;
    }

    HAL_NVIC_SetPriority((IRQn_Type) hardware->irq, hardware->irq_priority, 0U);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->irq);

    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF |
                                 UART_CLEAR_IDLEF);
    __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);
    __HAL_UART_ENABLE_IT(huart, UART_IT_RXFT);
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    __HAL_UART_DISABLE_IT(huart, UART_IT_TXFNF);
    return true;
}

static bool rx_ready(const UART_State *uart) {
    return __HAL_UART_GET_FLAG(uart_handle(uart), UART_FLAG_RXFNE);
}

static uint8_t read_byte(UART_State *uart) {
    return (uint8_t) (uart_handle(uart)->Instance->RDR & 0xFFU);
}

static bool tx_ready(const UART_State *uart) {
    return __HAL_UART_GET_FLAG(uart_handle(uart), UART_FLAG_TXFNF);
}

static void write_byte(UART_State *uart, uint8_t value) {
    uart_handle(uart)->Instance->TDR = value;
}

static void enable_rx_interrupts(UART_State *uart, bool enable) {
    UART_HandleTypeDef *huart = uart_handle(uart);

    if (enable) {
        __HAL_UART_ENABLE_IT(huart, UART_IT_RXFT);
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    } else {
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXFT);
        __HAL_UART_DISABLE_IT(huart, UART_IT_IDLE);
    }
}

static void enable_tx_interrupt(UART_State *uart, bool enable) {
    if (enable) {
        __HAL_UART_ENABLE_IT(uart_handle(uart), UART_IT_TXFNF);
    } else {
        __HAL_UART_DISABLE_IT(uart_handle(uart), UART_IT_TXFNF);
    }
}

static void drain_rx(UART_State *uart, UART_RxHandler rx_handler) {
    size_t length = 0U;

    while (rx_ready(uart)) {
        uart->rx_buffer[length++] = read_byte(uart);

        if (length == UART_RX_BUFFER_SIZE) {
            if (rx_handler != NULL) {
                rx_handler(uart->rx_buffer, length);
            }
            length = 0U;
        }
    }

    if ((length > 0U) && (rx_handler != NULL)) {
        rx_handler(uart->rx_buffer, length);
    }

    uart->rx_pending = false;
    enable_rx_interrupts(uart, true);
}

static void fill_tx(UART_State *uart) {
    while ((uart->tx_count > 0U) && tx_ready(uart)) {
        write_byte(uart, uart->tx_buffer[uart->tx_tail]);
        uart->tx_tail = (uart->tx_tail + 1U) % uart->tx_capacity;
        uart->tx_count--;
    }

    uart->tx_pending = false;
    enable_tx_interrupt(uart, uart->tx_count > 0U);
}

bool UART_Init(UART_State *uart, const UART_Hardware *hardware,
               uint8_t *tx_buffer, size_t tx_capacity) {
    if ((uart == NULL) || (hardware == NULL) || (tx_buffer == NULL) || (tx_capacity == 0U)) {
        return false;
    }

    memset(uart, 0, sizeof(*uart));
    uart->tx_buffer = tx_buffer;
    uart->tx_capacity = tx_capacity;

    return hardware_init(uart, hardware);
}

void UART_Service(UART_State *uart, UART_RxHandler rx_handler) {
    if ((uart == NULL) || (uart->platform_handle == NULL)) {
        return;
    }

    if (uart->rx_pending || rx_ready(uart)) {
        drain_rx(uart, rx_handler);
    }

    if (uart->tx_pending || ((uart->tx_count > 0U) && tx_ready(uart))) {
        fill_tx(uart);
    }
}

bool UART_Queue(UART_State *uart, const uint8_t *data, size_t length) {
    if ((uart == NULL) || (uart->platform_handle == NULL) || (data == NULL) || (length == 0U) ||
        (uart->tx_count > uart->tx_capacity) || (length > (uart->tx_capacity - uart->tx_count))) {
        return false;
    }

    for (size_t index = 0U; index < length; index++) {
        uart->tx_buffer[uart->tx_head] = data[index];
        uart->tx_head = (uart->tx_head + 1U) % uart->tx_capacity;
    }
    uart->tx_count += length;

    uart->tx_pending = true;
    enable_tx_interrupt(uart, true);
    return true;
}

void UART_IRQHandler(UART_State *uart) {
    if ((uart == NULL) || (uart->platform_handle == NULL)) {
        return;
    }

    UART_HandleTypeDef *huart = uart_handle(uart);
    const uint32_t flags = huart->Instance->ISR;

    if ((flags & (UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE)) != 0U) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
        uart->rx_pending = true;
    }

    if ((flags & UART_FLAG_IDLE) != 0U) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_IDLEF);
        uart->rx_pending = true;
        enable_rx_interrupts(uart, false);
    }

    if ((flags & UART_FLAG_RXFT) != 0U) {
        uart->rx_pending = true;
        enable_rx_interrupts(uart, false);
    }

    if (((flags & UART_FLAG_TXFNF) != 0U) && (uart->tx_count > 0U)) {
        uart->tx_pending = true;
        enable_tx_interrupt(uart, false);
    }
}
