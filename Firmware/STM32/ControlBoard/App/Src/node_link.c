#include "node_link.h"

#include "cobs/cobs.h"
#include "backplane_bus/protocol.h"
#include "mode.h"
#include "mode/support/chassis/backplane/bus.h"
#include "node_link/router.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#include <string.h>

#define NODE_LINK_TX_FRAME_COUNT 4U

static UART_State node_link_uart;
static UART_HandleTypeDef node_link_uart_handle;
static COBS_State node_link_cobs;
static uint8_t node_link_uart_tx_buffer[NODE_LINK_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];

#define NODE_LINK_ASSERT_BACKPLANE_PACKET_FITS(opcode, value, member) \
    _Static_assert(SIZE_##opcode <= (COBS_PACKET_MAX_SIZE - NODE_LINK_SIZE_HEADER), \
                   #opcode " exceeds the node link encapsulation capacity");

BACKPLANE_BUS_PROTOCOL_PACKETS(NODE_LINK_ASSERT_BACKPLANE_PACKET_FITS)

#undef NODE_LINK_ASSERT_BACKPLANE_PACKET_FITS

static void node_link_backplane_bus_receive(const NodeLink_Message *message) {
    if (Mode_Get() != MODE_SUPPORT_CHASSIS) {
        return;
    }

    const uint8_t *const data = (const uint8_t *) message->packet + NODE_LINK_SIZE_HEADER;
    (void) BackplaneBus_Receive(data, message->length - NODE_LINK_SIZE_HEADER);
}

/* Handlers for packets received from the chassis link belong here. */
static const NodeLink_Router node_link_router = {
    .backplane_bus = node_link_backplane_bus_receive,
};

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

static void node_link_packet_receive(const uint8_t *data, const size_t length) {
    (void) NodeLinkRouter_Dispatch(&node_link_router, data, length);
}

bool NodeLink_Init(void) {
    COBS_Init(&node_link_cobs, node_link_packet_receive);
    return UART_Init(&node_link_uart, &node_link_uart_hardware,
                     node_link_uart_tx_buffer, sizeof(node_link_uart_tx_buffer), NULL, 0U);
}

void NodeLink_Service(void) {
    UART_Service(&node_link_uart, node_link_uart_receive);
}

bool NodeLink_SendBackplanePacket(const uint8_t *data, const size_t length) {
    if ((data == NULL) || (length < BACKPLANE_BUS_SIZE_HEADER) ||
        (length > (COBS_PACKET_MAX_SIZE - NODE_LINK_SIZE_HEADER))) {
        return false;
    }

    uint8_t packet[COBS_PACKET_MAX_SIZE];
    packet[0] = NODE_LINK_BACKPLANE_BUS;
    memcpy(&packet[NODE_LINK_SIZE_HEADER], data, length);

    uint8_t frame[COBS_FRAME_MAX_SIZE];
    size_t frame_length;
    if (!COBS_Encode(packet, length + NODE_LINK_SIZE_HEADER,
                     frame, sizeof(frame), &frame_length)) {
        return false;
    }

    return UART_Queue(&node_link_uart, frame, frame_length);
}

void UART5_IRQHandler(void) {
    UART_IRQHandler(&node_link_uart);
}
