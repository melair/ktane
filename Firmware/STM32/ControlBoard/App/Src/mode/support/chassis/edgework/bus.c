#include "mode/support/chassis/edgework/bus.h"

#include "cobs/cobs.h"
#include "edgework/bus/router.h"
#include "mode/support/chassis/edgework/manager.h"
#include "mode.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

static const EdgeworkBus_Router edgework_bus_router = {
    .status = Edgework_StatusReceive,
};

static Bus_Data *bus_data(void) {
    return &mode_data.mode.chassis.bus;
}

static bool edgework_bus_uart_configure_clock(void) {
    __HAL_RCC_USART3_CONFIG(RCC_USART3CLKSOURCE_PCLK1);
    return true;
}

static void edgework_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART3_CLK_ENABLE();
}

static void edgework_bus_packet_receive(const uint8_t *data, const size_t length) {
    (void) EdgeworkBusRouter_Dispatch(&edgework_bus_router, data, length);
}

static void edgework_bus_uart_receive(const uint8_t *data, const size_t length) {
    COBS_Service(&bus_data()->cobs, data, length);
}

bool Bus_Init(void) {
    Bus_Data *const state = bus_data();
    const UART_Hardware hardware = {
        .uart_instance = USART3,
        .uart_handle = &state->uart_handle,
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

    COBS_Init(&state->cobs, edgework_bus_packet_receive);
    return UART_Init(&state->uart, &hardware,
                     state->uart_tx_buffer, sizeof(state->uart_tx_buffer));
}

void Bus_Service(void) {
    UART_Service(&bus_data()->uart, edgework_bus_uart_receive);
}

bool Bus_Send(const EdgeworkBus_Packet *packet, const size_t length) {
    uint8_t frame[COBS_FRAME_MAX_SIZE];
    size_t frame_length;

    if (!COBS_Encode((const uint8_t *) packet, length,
                     frame, sizeof(frame), &frame_length)) {
        return false;
    }

    return UART_Queue(&bus_data()->uart, frame, frame_length);
}

void USART3_IRQHandler(void) {
    UART_IRQHandler(&bus_data()->uart);
}
