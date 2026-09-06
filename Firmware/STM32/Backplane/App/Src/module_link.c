#include "module_link.h"

#include "cobs.h"
#include "sys/gpio.h"
#include "uart.h"
#include "uart_platform.h"

#define MODULE_LINK_TX_FRAME_COUNT 4U

typedef struct {
    UART_State uart;
    UART_HandleTypeDef uart_handle;
    COBS_State cobs;
    uint8_t uart_tx_buffer[MODULE_LINK_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];
} ModuleLink_State;

static ModuleLink_State front_module_link;
static ModuleLink_State rear_module_link;

static bool front_module_link_uart_configure_clock(void) {
    __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK1);
    return true;
}

static void front_module_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
}

static bool rear_module_link_uart_configure_clock(void) {
    /* USART4 is clocked directly from PCLK1 on the STM32G070. */
    return true;
}

static void rear_module_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART4_CLK_ENABLE();
}

static const UART_Hardware front_module_link_uart_hardware = {
    .uart_instance = USART1,
    .uart_handle = &front_module_link.uart_handle,
    .baud_rate = 115200U,
    .rx = {FRONT_UART_RX_GPIO_Port, FRONT_UART_RX_Pin},
    .rx_alternate = GPIO_AF1_USART1,
    .tx = {FRONT_UART_TX_GPIO_Port, FRONT_UART_TX_Pin},
    .tx_alternate = GPIO_AF1_USART1,
    .irq = USART1_IRQn,
    .irq_priority = 1U,
    .configure_clock = front_module_link_uart_configure_clock,
    .enable_peripheral_clock = front_module_link_uart_enable_peripheral_clock,
};

static const UART_Hardware rear_module_link_uart_hardware = {
    .uart_instance = USART4,
    .uart_handle = &rear_module_link.uart_handle,
    .baud_rate = 115200U,
    .rx = {REAR_UART_RX_GPIO_Port, REAR_UART_RX_Pin},
    .rx_alternate = GPIO_AF4_USART4,
    .tx = {REAR_UART_TX_GPIO_Port, REAR_UART_TX_Pin},
    .tx_alternate = GPIO_AF4_USART4,
    .irq = USART3_4_IRQn,
    .irq_priority = 1U,
    .configure_clock = rear_module_link_uart_configure_clock,
    .enable_peripheral_clock = rear_module_link_uart_enable_peripheral_clock,
};

static bool module_link_init(ModuleLink_State *module_link, const UART_Hardware *hardware) {
    COBS_Init(&module_link->cobs, NULL);
    return UART_Init(&module_link->uart, hardware,
                     module_link->uart_tx_buffer, sizeof(module_link->uart_tx_buffer));
}

static void front_module_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&front_module_link.cobs, data, length);
}

static void rear_module_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&rear_module_link.cobs, data, length);
}

bool ModuleLink_Init(void) {
    return module_link_init(&front_module_link, &front_module_link_uart_hardware) &&
           module_link_init(&rear_module_link, &rear_module_link_uart_hardware);
}

void ModuleLink_Service(void) {
    UART_Service(&front_module_link.uart, front_module_link_uart_receive);
    UART_Service(&rear_module_link.uart, rear_module_link_uart_receive);
}

void ModuleLink_Front_IRQHandler(void) {
    UART_IRQHandler(&front_module_link.uart);
}

void ModuleLink_Rear_IRQHandler(void) {
    UART_IRQHandler(&rear_module_link.uart);
}
