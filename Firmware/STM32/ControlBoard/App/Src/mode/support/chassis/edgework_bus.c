#include "mode/support/chassis/edgework_bus.h"

#include "cobs/cobs.h"
#include "edgework_bus/router.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define EDGEWORK_BUS_TX_FRAME_COUNT 4U

static UART_State edgework_bus_uart;
static UART_HandleTypeDef edgework_bus_uart_handle;
static COBS_State edgework_bus_cobs;
static uint8_t edgework_bus_uart_tx_buffer[EDGEWORK_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

/* Handlers for packets received by the control board belong here. */
static const EdgeworkBus_Router edgework_bus_router = {0};

static bool edgework_bus_uart_configure_clock(void) {
    __HAL_RCC_USART3_CONFIG(RCC_USART3CLKSOURCE_PCLK1);
    return true;
}

static void edgework_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART3_CLK_ENABLE();
}

static const UART_Hardware edgework_bus_uart_hardware = {
    .uart_instance = USART3,
    .uart_handle = &edgework_bus_uart_handle,
    .baud_rate = 115200U,
    .rx = {GPIO_A3_Port, GPIO_A3_Pin},
    .rx_alternate = GPIO_AF7_USART3,
    .tx = {GPIO_A2_Port, GPIO_A2_Pin},
    .tx_alternate = GPIO_AF7_USART3,
    .driver_enable = {GPIO_A1_Port, GPIO_A1_Pin},
    .driver_enable_alternate = GPIO_AF7_USART3,
    .driver_enable_active_low = false,
    .irq = USART3_IRQn,
    .irq_priority = 5U,
    .configure_clock = edgework_bus_uart_configure_clock,
    .enable_peripheral_clock = edgework_bus_uart_enable_peripheral_clock,
};

static void edgework_bus_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&edgework_bus_cobs, data, length);
}

static void edgework_bus_packet_receive(const uint8_t *data, const size_t length) {
    (void) EdgeworkBusRouter_Dispatch(&edgework_bus_router, data, length);
}

bool EdgeworkBus_Init(void) {
    COBS_Init(&edgework_bus_cobs, edgework_bus_packet_receive);
    return UART_Init(&edgework_bus_uart, &edgework_bus_uart_hardware,
                     edgework_bus_uart_tx_buffer, sizeof(edgework_bus_uart_tx_buffer));
}

void EdgeworkBus_Service(void) {
    UART_Service(&edgework_bus_uart, edgework_bus_uart_receive);
}

void USART3_IRQHandler(void) {
    UART_IRQHandler(&edgework_bus_uart);
}
