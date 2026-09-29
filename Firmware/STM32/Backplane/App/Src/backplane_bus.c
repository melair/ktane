#include "backplane_bus.h"

#include "backplane_bus/router.h"
#include "cobs/cobs.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define BACKPLANE_BUS_TX_FRAME_COUNT 4U

static UART_State backplane_bus_uart;
static UART_HandleTypeDef backplane_bus_uart_handle;
static COBS_State backplane_bus_cobs;
static uint8_t backplane_bus_uart_tx_buffer[BACKPLANE_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

/* Handlers for packets received by a backplane belong here. */
static const BackplaneBus_Router backplane_bus_router = {0};

static bool backplane_bus_uart_configure_clock(void) {
    /* USART3 is clocked directly from PCLK1 on the STM32G070. */
    return true;
}

static void backplane_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART3_CLK_ENABLE();
}

static const UART_Hardware backplane_bus_uart_hardware = {
    .uart_instance = USART3,
    .uart_handle = &backplane_bus_uart_handle,
    .baud_rate = 115200U,
    .rx = {BUS_RX_GPIO_Port, BUS_RX_Pin},
    .rx_alternate = GPIO_AF4_USART3,
    .tx = {BUS_TX_GPIO_Port, BUS_TX_Pin},
    .tx_alternate = GPIO_AF4_USART3,
    .driver_enable = {BUS_DE_GPIO_Port, BUS_DE_Pin},
    .driver_enable_alternate = GPIO_AF4_USART3,
    .driver_enable_active_low = false,
    .irq = USART3_4_IRQn,
    .irq_priority = 1U,
    .configure_clock = backplane_bus_uart_configure_clock,
    .enable_peripheral_clock = backplane_bus_uart_enable_peripheral_clock,
};

static void backplane_bus_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&backplane_bus_cobs, data, length);
}

static void backplane_bus_packet_receive(const uint8_t *data, const size_t length) {
    (void) BackplaneBusRouter_Dispatch(&backplane_bus_router, data, length);
}

bool BackplaneBus_Init(void) {
    COBS_Init(&backplane_bus_cobs, backplane_bus_packet_receive);
    return UART_Init(&backplane_bus_uart, &backplane_bus_uart_hardware,
                     backplane_bus_uart_tx_buffer, sizeof(backplane_bus_uart_tx_buffer));
}

void BackplaneBus_Service(void) {
    UART_Service(&backplane_bus_uart, backplane_bus_uart_receive);
}

void BackplaneBus_IRQHandler(void) {
    UART_IRQHandler(&backplane_bus_uart);
}
