#include "node_link.h"

#include "cobs/cobs.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define NODE_LINK_TX_FRAME_COUNT 4U

static UART_State node_link_uart;
static UART_HandleTypeDef node_link_uart_handle;
static COBS_State node_link_cobs;
static uint8_t node_link_uart_tx_buffer[NODE_LINK_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

static bool node_link_uart_configure_clock(void) {
    __HAL_RCC_UART5_CONFIG(RCC_UART5CLKSOURCE_PLL2Q);
    return true;
}

static void node_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_UART5_CLK_ENABLE();
}

static const UART_Hardware node_link_uart_hardware = {
    .uart_instance = UART5,
    .uart_handle = &node_link_uart_handle,
    .baud_rate = 115200U,
    .rx = {NODE_LINK_RX_Port, NODE_LINK_RX_Pin},
    .rx_alternate = GPIO_AF14_UART5,
    .tx = {NODE_LINK_TX_Port, NODE_LINK_TX_Pin},
    .tx_alternate = GPIO_AF14_UART5,
    .irq = UART5_IRQn,
    .irq_priority = 5U,
    .configure_clock = node_link_uart_configure_clock,
    .enable_peripheral_clock = node_link_uart_enable_peripheral_clock,
};

static void node_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&node_link_cobs, data, length);
}

bool NodeLink_Init(void) {
    COBS_Init(&node_link_cobs, NULL);
    return UART_Init(&node_link_uart, &node_link_uart_hardware,
                     node_link_uart_tx_buffer, sizeof(node_link_uart_tx_buffer));
}

void NodeLink_Service(void) {
    UART_Service(&node_link_uart, node_link_uart_receive);
}

void UART5_IRQHandler(void) {
    UART_IRQHandler(&node_link_uart);
}
