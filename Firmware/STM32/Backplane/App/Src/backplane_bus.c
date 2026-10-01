#include "backplane_bus.h"

#include "backplane.h"
#include "backplane_bus/router.h"
#include "backplane_bus/protocol.h"
#include "cobs/cobs.h"
#include "node_link.h"
#include "power.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define BACKPLANE_BUS_TX_FRAME_COUNT 4U
#define BACKPLANE_BUS_REMOTE_MANAGEMENT_FALLBACK_MS 1000U

static UART_State backplane_bus_uart;
static UART_HandleTypeDef backplane_bus_uart_handle;
static COBS_State backplane_bus_cobs;
static uint8_t backplane_bus_uart_tx_buffer[BACKPLANE_BUS_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];
static uint8_t backplane_bus_uart_rx_ring[UART_RX_RING_DEFAULT_CAPACITY];
static uint32_t backplane_bus_remote_management_deadline_ms;
static bool backplane_bus_inquiry_seen;
static bool backplane_bus_local_management_enabled;

static void backplane_bus_inquiry_receive(const BackplaneBus_Message *message);
static void backplane_bus_set_enabled_receive(const BackplaneBus_Message *message);
static void backplane_bus_set_current_limit_receive(const BackplaneBus_Message *message);
static void backplane_bus_send_status(Power_ChannelId channel,
                                      bool end_of_response);
static bool BackplaneBus_Send(const uint8_t *data, size_t length);

/* Handlers for packets received by a backplane belong here. */
static const BackplaneBus_Router backplane_bus_router = {
    .inquiry = backplane_bus_inquiry_receive,
    .set_enabled = backplane_bus_set_enabled_receive,
    .set_current_limit = backplane_bus_set_current_limit_receive,
};

static bool backplane_bus_time_reached(const uint32_t now_ms, const uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static bool backplane_bus_uart_configure_clock(void) {
    /* USART3 is clocked directly from PCLK1 on the STM32G070. */
    return true;
}

static void backplane_bus_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART3_CLK_ENABLE();
}

static void backplane_bus_uart_error(void) {
    COBS_Init(&backplane_bus_cobs, backplane_bus_cobs.packet_handler);
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
    .rx_error_handler = backplane_bus_uart_error,
};

static void backplane_bus_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&backplane_bus_cobs, data, length);
}

static void backplane_bus_packet_receive(const uint8_t *data, const size_t length) {

    (void) NodeLink_ForwardBackplanePacket(data, length);
    BackplaneBus_ProcessPacket(data, length);
}

static void backplane_bus_inquiry_receive(const BackplaneBus_Message *message) {
    if (Backplane_GetLocation() != BACKPLANE_LOCATION_CHASSIS) {
        backplane_bus_inquiry_seen = true;
        Power_SetLocallyManaged(false);
    }

    backplane_bus_send_status(POWER_CHANNEL_FRONT, false);
    backplane_bus_send_status(POWER_CHANNEL_REAR, true);
}

static void backplane_bus_set_enabled_receive(const BackplaneBus_Message *message) {
    const Power_ChannelId channel = message->packet->set_enabled.flags.port
                                        ? POWER_CHANNEL_REAR
                                        : POWER_CHANNEL_FRONT;

    (void) Power_SetEnabled(channel, message->packet->set_enabled.flags.enabled != 0U);
    backplane_bus_send_status(channel, true);
}

static void backplane_bus_set_current_limit_receive(const BackplaneBus_Message *message) {
    const Power_ChannelId channel = message->packet->set_current_limit.flags.port
                                        ? POWER_CHANNEL_REAR
                                        : POWER_CHANNEL_FRONT;

    (void) Power_SetCurrentLimit(channel, message->packet->set_current_limit.deciamps);
    backplane_bus_send_status(channel, true);
}

static void backplane_bus_send_status(const Power_ChannelId channel,
                                      const bool end_of_response) {
    BackplaneBus_Packet response = {0};

    response.header.address = Backplane_GetLocation();
    response.header.opcode = BACKPLANE_BUS_STATUS;
    response.header.flags.eor = end_of_response;
    response.status.flags.port = (unsigned int) channel;
    response.status.flags.power_state = (unsigned int) Power_GetState(channel);
    response.status.flags.module_detected = Power_IsModuleDetected(channel);
    response.status.current_limit_deciamps = Power_GetCurrentLimit(channel);
    response.status.current_milliamps = Power_GetCurrent(channel);
    response.status.peak_current_milliamps = Power_GetPeakCurrent(channel);

    (void) BackplaneBus_Send((const uint8_t *) &response, SIZE_BACKPLANE_BUS_STATUS);
}

void BackplaneBus_ProcessPacket(const uint8_t *data, const size_t length) {
    const BackplaneBus_Packet *const packet = (const BackplaneBus_Packet *) data;
    if (packet->header.address != Backplane_GetLocation()) {
        return;
    }

    (void) BackplaneBusRouter_Dispatch(&backplane_bus_router, data, length);
}

bool BackplaneBus_Init(void) {
    COBS_Init(&backplane_bus_cobs, backplane_bus_packet_receive);
    backplane_bus_inquiry_seen = false;
    backplane_bus_local_management_enabled =
        Backplane_GetLocation() == BACKPLANE_LOCATION_CHASSIS;
    backplane_bus_remote_management_deadline_ms =
        HAL_GetTick() + BACKPLANE_BUS_REMOTE_MANAGEMENT_FALLBACK_MS;

    return UART_Init(&backplane_bus_uart, &backplane_bus_uart_hardware,
                     backplane_bus_uart_tx_buffer, sizeof(backplane_bus_uart_tx_buffer),
                     backplane_bus_uart_rx_ring, sizeof(backplane_bus_uart_rx_ring));
}

void BackplaneBus_Service(void) {
    UART_Service(&backplane_bus_uart, backplane_bus_uart_receive);

    if ((Backplane_GetLocation() != BACKPLANE_LOCATION_CHASSIS) &&
        !backplane_bus_inquiry_seen && !backplane_bus_local_management_enabled &&
        backplane_bus_time_reached(HAL_GetTick(), backplane_bus_remote_management_deadline_ms)) {
        backplane_bus_local_management_enabled = true;
        Power_SetLocallyManaged(true);
    }
}

bool BackplaneBus_SendPhysical(const uint8_t *data, const size_t length) {
    uint8_t frame[COBS_FRAME_MAX_SIZE];
    size_t frame_length;

    if (!COBS_Encode(data, length, frame, sizeof(frame), &frame_length)) {
        return false;
    }

    return UART_Queue(&backplane_bus_uart, frame, frame_length);
}

static bool BackplaneBus_Send(const uint8_t *data, const size_t length) {
    const bool physical_queued = BackplaneBus_SendPhysical(data, length);

    if (Backplane_GetLocation() != BACKPLANE_LOCATION_CHASSIS) {
        return physical_queued;
    }

    const bool node_links_queued = NodeLink_ForwardBackplanePacket(data, length);
    return physical_queued && node_links_queued;
}

void BackplaneBus_IRQHandler(void) {
    UART_IRQHandler(&backplane_bus_uart);
}
