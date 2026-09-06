#include "chassis_link.h"

#include "cobs.h"
#include "sys/gpio.h"
#include "uart.h"
#include "uart_platform.h"

#define CHASSIS_LINK_TX_FRAME_COUNT 4U

static UART_State chassis_link_uart;
static UART_HandleTypeDef chassis_link_uart_handle;
static COBS_State chassis_link_cobs;
static uint8_t chassis_link_uart_tx_buffer[CHASSIS_LINK_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

static bool chassis_link_uart_configure_clock(void) {
    __HAL_RCC_UART5_CONFIG(RCC_UART5CLKSOURCE_PLL2Q);
    return true;
}

static void chassis_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_UART5_CLK_ENABLE();
}

static const UART_Hardware chassis_link_uart_hardware = {
    .uart_instance = UART5,
    .uart_handle = &chassis_link_uart_handle,
    .baud_rate = 115200U,
    .rx = {CHASSIS_LINK_RX_Port, CHASSIS_LINK_RX_Pin},
    .rx_alternate = GPIO_AF14_UART5,
    .tx = {CHASSIS_LINK_TX_Port, CHASSIS_LINK_TX_Pin},
    .tx_alternate = GPIO_AF14_UART5,
    .irq = UART5_IRQn,
    .irq_priority = 5U,
    .configure_clock = chassis_link_uart_configure_clock,
    .enable_peripheral_clock = chassis_link_uart_enable_peripheral_clock,
};

static void chassis_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&chassis_link_cobs, data, length);
}

bool ChassisLink_Init(void) {
    COBS_Init(&chassis_link_cobs, NULL);
    return UART_Init(&chassis_link_uart, &chassis_link_uart_hardware,
                     chassis_link_uart_tx_buffer, sizeof(chassis_link_uart_tx_buffer));
}

void ChassisLink_Service(void) {
    UART_Service(&chassis_link_uart, chassis_link_uart_receive);
}

void UART5_IRQHandler(void) {
    UART_IRQHandler(&chassis_link_uart);
}
