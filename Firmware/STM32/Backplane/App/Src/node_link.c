#include "node_link.h"

#include "backplane.h"
#include "cobs/cobs.h"
#include "node_link/protocol.h"
#include "power.h"
#include "sys/gpio.h"
#include "uart/uart.h"
#include "uart/uart_platform.h"

#define NODE_LINK_ANNOUNCEMENT_INTERVAL_MS 200U
#define NODE_LINK_TX_FRAME_COUNT 4U

_Static_assert(SIZE_NODE_LINK_ANNOUNCEMENT <= COBS_PACKET_MAX_SIZE,
               "Node link announcement exceeds the maximum COBS packet size");

typedef struct {
    UART_State uart;
    UART_HandleTypeDef uart_handle;
    COBS_State cobs;
    uint8_t uart_tx_buffer[NODE_LINK_TX_FRAME_COUNT * COBS_FRAME_MAX_SIZE];
} NodeLink_State;

static NodeLink_State front_node_link;
static NodeLink_State rear_node_link;
static uint32_t next_announcement_ms;

static bool time_reached(const uint32_t now_ms, const uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static bool front_node_link_uart_configure_clock(void) {
    __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK1);
    return true;
}

static void front_node_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
}

static bool rear_node_link_uart_configure_clock(void) {
    /* USART4 is clocked directly from PCLK1 on the STM32G070. */
    return true;
}

static void rear_node_link_uart_enable_peripheral_clock(void) {
    __HAL_RCC_USART4_CLK_ENABLE();
}

static const UART_Hardware front_node_link_uart_hardware = {
    .uart_instance = USART1,
    .uart_handle = &front_node_link.uart_handle,
    .baud_rate = 115200U,
    .rx = {FRONT_UART_RX_GPIO_Port, FRONT_UART_RX_Pin},
    .rx_alternate = GPIO_AF1_USART1,
    .tx = {FRONT_UART_TX_GPIO_Port, FRONT_UART_TX_Pin},
    .tx_alternate = GPIO_AF1_USART1,
    .irq = USART1_IRQn,
    .irq_priority = 1U,
    .configure_clock = front_node_link_uart_configure_clock,
    .enable_peripheral_clock = front_node_link_uart_enable_peripheral_clock,
};

static const UART_Hardware rear_node_link_uart_hardware = {
    .uart_instance = USART4,
    .uart_handle = &rear_node_link.uart_handle,
    .baud_rate = 115200U,
    .rx = {REAR_UART_RX_GPIO_Port, REAR_UART_RX_Pin},
    .rx_alternate = GPIO_AF4_USART4,
    .tx = {REAR_UART_TX_GPIO_Port, REAR_UART_TX_Pin},
    .tx_alternate = GPIO_AF4_USART4,
    .irq = USART3_4_IRQn,
    .irq_priority = 1U,
    .configure_clock = rear_node_link_uart_configure_clock,
    .enable_peripheral_clock = rear_node_link_uart_enable_peripheral_clock,
};

static bool node_link_init(NodeLink_State *node_link, const UART_Hardware *hardware) {
    COBS_Init(&node_link->cobs, NULL);
    return UART_Init(&node_link->uart, hardware,
                     node_link->uart_tx_buffer, sizeof(node_link->uart_tx_buffer));
}

static uint8_t node_link_chassis_location(const Power_ChannelId channel_id) {
    const BackplaneLocation backplane_location = Backplane_GetLocation();

    if (backplane_location == BACKPLANE_LOCATION_UNKNOWN) {
        return NODE_CHASSIS_LOCATION_UNKNOWN;
    }
    if (backplane_location == BACKPLANE_LOCATION_CHASSIS) {
        return NODE_CHASSIS_LOCATION_CHASSIS;
    }
    if (backplane_location > BACKPLANE_LOCATION_5) {
        return NODE_CHASSIS_LOCATION_UNKNOWN;
    }

    return (uint8_t) ((backplane_location << 1U) | (uint8_t) channel_id);
}

static void node_link_send_announcement(NodeLink_State *node_link,
                                        const Power_ChannelId channel_id) {
    NodeLink_Packet packet = {0};
    packet.header.opcode = NODE_LINK_ANNOUNCEMENT;
    packet.announcement.chassis_location = node_link_chassis_location(channel_id);
    packet.announcement.current_limit_deciamps = Power_GetCurrentLimit(channel_id);

    uint8_t frame[COBS_FRAME_MAX_SIZE];
    size_t frame_length;
    if (COBS_Encode((const uint8_t *) &packet, SIZE_NODE_LINK_ANNOUNCEMENT,
                    frame, sizeof(frame), &frame_length)) {
        (void) UART_Queue(&node_link->uart, frame, frame_length);
    }
}

static void front_node_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&front_node_link.cobs, data, length);
}

static void rear_node_link_uart_receive(const uint8_t *data, size_t length) {
    COBS_Service(&rear_node_link.cobs, data, length);
}

bool NodeLink_Init(void) {
    if (!node_link_init(&front_node_link, &front_node_link_uart_hardware) ||
        !node_link_init(&rear_node_link, &rear_node_link_uart_hardware)) {
        return false;
    }

    next_announcement_ms = HAL_GetTick() + NODE_LINK_ANNOUNCEMENT_INTERVAL_MS;
    return true;
}

void NodeLink_Service(void) {
    UART_Service(&front_node_link.uart, front_node_link_uart_receive);
    UART_Service(&rear_node_link.uart, rear_node_link_uart_receive);

    const uint32_t now_ms = HAL_GetTick();
    if (time_reached(now_ms, next_announcement_ms)) {
        next_announcement_ms = now_ms + NODE_LINK_ANNOUNCEMENT_INTERVAL_MS;
        node_link_send_announcement(&front_node_link, POWER_CHANNEL_FRONT);
        node_link_send_announcement(&rear_node_link, POWER_CHANNEL_REAR);
    }
}

void NodeLink_Front_IRQHandler(void) {
    UART_IRQHandler(&front_node_link.uart);
}

void NodeLink_Rear_IRQHandler(void) {
    UART_IRQHandler(&rear_node_link.uart);
}
