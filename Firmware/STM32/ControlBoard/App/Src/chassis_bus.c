#include "chassis_bus.h"

#include "cobs.h"
#include "sys/gpio.h"
#include "uart.h"
#include "uart_platform.h"

#define CHASSIS_BUS_TX_FRAME_COUNT 4U

static UART_State chassis_bus_uart;
static UART_HandleTypeDef chassis_bus_uart_handle;
static COBS_State chassis_bus_cobs;
static uint8_t chassis_bus_uart_tx_buffer[CHASSIS_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

static bool chassis_bus_uart_configure_clock(void) {
    __HAL_RCC_UART5_CONFIG(RCC_UART5CLKSOURCE_PLL2Q);
    return true;
}

static void chassis_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_UART5_CLK_ENABLE();
}

static const UART_Hardware chassis_bus_uart_hardware = {
    .uart_instance = UART5,
    .uart_handle = &chassis_bus_uart_handle,
    .baud_rate = 115200U,
    .rx = {CHASSIS_BUS_RX_Port, CHASSIS_BUS_RX_Pin},
    .rx_alternate = GPIO_AF14_UART5,
    .tx = {CHASSIS_BUS_TX_Port, CHASSIS_BUS_TX_Pin},
    .tx_alternate = GPIO_AF14_UART5,
    .irq = UART5_IRQn,
    .irq_priority = 5U,
    .configure_clock = chassis_bus_uart_configure_clock,
    .enable_peripheral_clock = chassis_bus_uart_enable_peripheral_clock,
};

static void chassis_bus_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&chassis_bus_cobs, data, length);
}

bool ChassisBus_Init(void) {
    COBS_Init(&chassis_bus_cobs, NULL);
    return UART_Init(&chassis_bus_uart, &chassis_bus_uart_hardware,
                     chassis_bus_uart_tx_buffer, sizeof(chassis_bus_uart_tx_buffer));
}

void ChassisBus_Service(void) {
    UART_Service(&chassis_bus_uart, chassis_bus_uart_receive);
}

void UART5_IRQHandler(void) {
    UART_IRQHandler(&chassis_bus_uart);
}
