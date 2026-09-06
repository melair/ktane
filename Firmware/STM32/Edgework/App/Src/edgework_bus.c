#include "edgework_bus.h"

#include "cobs.h"
#include "sys/gpio.h"
#include "uart.h"
#include "uart_platform.h"

#define EDGEWORK_BUS_TX_FRAME_COUNT 4U

static UART_State edgework_bus_uart;
static UART_HandleTypeDef edgework_bus_uart_handle;
static COBS_State edgework_bus_cobs;
static uint8_t edgework_bus_uart_tx_buffer[EDGEWORK_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

static bool edgework_bus_uart_configure_clock(void) {
    __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK1);
    return true;
}

static void edgework_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
}

static const UART_Hardware edgework_bus_uart_hardware = {
    .uart_instance = USART1,
    .uart_handle = &edgework_bus_uart_handle,
    .baud_rate = 115200U,
    .rx = {BUS_RX_Port, BUS_RX_Pin},
    .rx_alternate = GPIO_AF0_USART1,
    .tx = {BUS_TX_Port, BUS_TX_Pin},
    .tx_alternate = GPIO_AF0_USART1,
    .driver_enable = {BUS_DE_Port, BUS_DE_Pin},
    .driver_enable_alternate = GPIO_AF4_USART1,
    .driver_enable_active_low = false,
    .irq = USART1_IRQn,
    .irq_priority = 1U,
    .configure_clock = edgework_bus_uart_configure_clock,
    .enable_peripheral_clock = edgework_bus_uart_enable_peripheral_clock,
};

static void edgework_bus_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&edgework_bus_cobs, data, length);
}

bool EdgeworkBus_Init(void) {
    COBS_Init(&edgework_bus_cobs, NULL);
    return UART_Init(&edgework_bus_uart, &edgework_bus_uart_hardware,
                     edgework_bus_uart_tx_buffer, sizeof(edgework_bus_uart_tx_buffer));
}

void EdgeworkBus_Service(void) {
    UART_Service(&edgework_bus_uart, edgework_bus_uart_receive);
}

void USART1_IRQHandler(void) {
    UART_IRQHandler(&edgework_bus_uart);
}
