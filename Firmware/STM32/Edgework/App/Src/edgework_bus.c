#include "edgework_bus.h"

#include "mode.h"
#include "slot.h"
#include "cobs/cobs.h"
#include "edgework/bus/router.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define EDGEWORK_BUS_TX_FRAME_COUNT 4U

static UART_State edgework_bus_uart;
static UART_HandleTypeDef edgework_bus_uart_handle;
static COBS_State edgework_bus_cobs;
static uint8_t edgework_bus_uart_tx_buffer[EDGEWORK_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

static bool edgework_bus_send(const EdgeworkBus_Packet *packet, const size_t length) {
    uint8_t frame[COBS_FRAME_MAX_SIZE];
    size_t frame_length;

    if (!COBS_Encode((const uint8_t *) packet, length,
                     frame, sizeof(frame), &frame_length)) {
        return false;
    }

    return UART_Queue(&edgework_bus_uart, frame, frame_length);
}

static void edgework_bus_inquiry_receive(const EdgeworkBus_Message *message) {
    if (message->address != Slot_Get()) {
        return;
    }

    EdgeworkBus_Packet response = {0};
    response.header.address = message->address;
    response.header.opcode = EDGEWORK_BUS_STATUS;
    response.status.mode = Mode_Get();
    response.status.data = Mode_State();
    response.status.state = Mode_FSMState();

    (void) edgework_bus_send(&response, SIZE_EDGEWORK_BUS_STATUS);
}

static void edgework_bus_display_receive(const EdgeworkBus_Message *message) {
    if (message->address != Slot_Get()) {
        return;
    }

    (void) Mode_Display(message->packet->display.data);
}

static void edgework_bus_set_mode_receive(const EdgeworkBus_Message *message) {
    if (message->address != Slot_Get()) {
        return;
    }

    Mode_Set(message->packet->set_mode.new_mode);
}

static void edgework_bus_set_slot_address_receive(const EdgeworkBus_Message *message) {
    if (message->address != Slot_Get() && message->address != SLOT_UNKNOWN) {
        return;
    }

    Slot_Set(message->packet->set_slot_address.new_address);
}

static const EdgeworkBus_Router edgework_bus_router = {
    .inquiry = edgework_bus_inquiry_receive,
    .display = edgework_bus_display_receive,
    .set_mode = edgework_bus_set_mode_receive,
    .set_slot_address = edgework_bus_set_slot_address_receive,
};

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

static void edgework_bus_uart_receive(const uint8_t *data, const size_t length) {
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

void USART1_IRQHandler(void) {
    UART_IRQHandler(&edgework_bus_uart);
}
