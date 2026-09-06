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
    /* USART3 is clocked directly from PCLK1 on the STM32G070. */
    return true;
}

static void chassis_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART3_CLK_ENABLE();
}

static const UART_Hardware chassis_bus_uart_hardware = {
    .uart_instance = USART3,
    .uart_handle = &chassis_bus_uart_handle,
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

void ChassisBus_IRQHandler(void) {
    UART_IRQHandler(&chassis_bus_uart);
}
