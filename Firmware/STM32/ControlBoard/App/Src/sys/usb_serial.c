#include "sys/usb_serial.h"

#include "main.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdbool.h>
#include <string.h>

#ifndef USB_SERIAL_VENDOR_ID
/* ST's development VID/PID for its CDC example. Replace for a released product. */
#define USB_SERIAL_VENDOR_ID 0x0483U
#endif
#ifndef USB_SERIAL_PRODUCT_ID
#define USB_SERIAL_PRODUCT_ID 0x5740U
#endif

#define USB_EP0_MAX_PACKET_SIZE 64U
#define USB_CDC_DATA_MAX_PACKET_SIZE 64U
#define USB_CDC_COMMAND_MAX_PACKET_SIZE 8U

#define USB_CDC_OUT_EP 0x01U
#define USB_CDC_IN_EP 0x81U
#define USB_CDC_COMMAND_EP 0x82U

#define USB_TX_RING_SIZE 1024U
#define USB_CAN_MAX_PAYLOAD_SIZE 64U
#define USB_CAN_FRAME_OVERHEAD 14U
#define USB_CAN_MAX_FRAME_SIZE (USB_CAN_FRAME_OVERHEAD + USB_CAN_MAX_PAYLOAD_SIZE)

#define USB_DESCRIPTOR_TYPE_DEVICE 0x01U
#define USB_DESCRIPTOR_TYPE_CONFIGURATION 0x02U
#define USB_DESCRIPTOR_TYPE_STRING 0x03U

#define USB_REQUEST_GET_STATUS 0x00U
#define USB_REQUEST_CLEAR_FEATURE 0x01U
#define USB_REQUEST_SET_FEATURE 0x03U
#define USB_REQUEST_SET_ADDRESS 0x05U
#define USB_REQUEST_GET_DESCRIPTOR 0x06U
#define USB_REQUEST_GET_CONFIGURATION 0x08U
#define USB_REQUEST_SET_CONFIGURATION 0x09U
#define USB_REQUEST_GET_INTERFACE 0x0AU
#define USB_REQUEST_SET_INTERFACE 0x0BU

#define USB_REQUEST_TYPE_MASK 0x60U
#define USB_REQUEST_TYPE_STANDARD 0x00U
#define USB_REQUEST_TYPE_CLASS 0x20U
#define USB_REQUEST_RECIPIENT_MASK 0x1FU
#define USB_REQUEST_RECIPIENT_DEVICE 0x00U
#define USB_REQUEST_RECIPIENT_INTERFACE 0x01U
#define USB_REQUEST_RECIPIENT_ENDPOINT 0x02U
#define USB_REQUEST_DIRECTION_IN 0x80U
#define USB_FEATURE_ENDPOINT_HALT 0x00U

#define USB_CDC_SET_LINE_CODING 0x20U
#define USB_CDC_GET_LINE_CODING 0x21U
#define USB_CDC_SET_CONTROL_LINE_STATE 0x22U
#define USB_CDC_SEND_BREAK 0x23U

typedef enum {
    USB_EP0_IDLE,
    USB_EP0_DATA_IN,
    USB_EP0_DATA_OUT,
    USB_EP0_STATUS_IN,
    USB_EP0_STATUS_OUT,
} USB_EP0_State;

typedef struct {
    uint8_t bmRequest;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} USB_SetupPacket;

typedef struct {
    PCD_HandleTypeDef handle;
    volatile bool configured;
    volatile bool txBusy;
    volatile bool flushPending;
    bool txNeedsZlp;
    bool ep0NeedsZlp;
    const uint8_t *ep0TxData;
    uint16_t ep0TxRemaining;
    USB_EP0_State ep0State;
    uint8_t configuration;
    uint8_t pendingControlRequest;
    uint16_t controlLineState;
    uint8_t descriptorBuffer[USB_EP0_MAX_PACKET_SIZE];
    uint8_t lineCoding[7];
    uint8_t bulkOutBuffer[USB_CDC_DATA_MAX_PACKET_SIZE];
    uint8_t txBuffer[USB_CDC_DATA_MAX_PACKET_SIZE];
    uint8_t txRing[USB_TX_RING_SIZE];
    uint16_t txHead;
    uint16_t txTail;
    uint32_t droppedPackets;
} USB_SerialState;

static USB_SerialState usb;

static const uint8_t deviceDescriptor[] = {
    0x12, USB_DESCRIPTOR_TYPE_DEVICE,
    0x00, 0x02,                         /* USB 2.0 */
    0x02, 0x02, 0x00,                   /* CDC device */
    USB_EP0_MAX_PACKET_SIZE,
    (uint8_t) USB_SERIAL_VENDOR_ID,
    (uint8_t) (USB_SERIAL_VENDOR_ID >> 8U),
    (uint8_t) USB_SERIAL_PRODUCT_ID,
    (uint8_t) (USB_SERIAL_PRODUCT_ID >> 8U),
    0x01, 0x01,                         /* Device release 1.01 */
    0x01, 0x02, 0x03,                   /* Manufacturer, product, serial */
    0x01,                               /* One configuration */
};

static const uint8_t configurationDescriptor[] = {
    /* Configuration descriptor. */
    0x09, USB_DESCRIPTOR_TYPE_CONFIGURATION,
    0x43, 0x00,                         /* Total length: 67 bytes */
    0x02, 0x01, 0x00,
    0x80, 0x32,                         /* Bus powered, 100 mA */

    /* CDC communication interface. */
    0x09, 0x04, 0x00, 0x00, 0x01,
    0x02, 0x02, 0x01, 0x00,

    /* CDC header, call-management, ACM and union functional descriptors. */
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC notification endpoint. */
    0x07, 0x05, USB_CDC_COMMAND_EP, 0x03,
    USB_CDC_COMMAND_MAX_PACKET_SIZE, 0x00, 0x10,

    /* CDC data interface. */
    0x09, 0x04, 0x01, 0x00, 0x02,
    0x0A, 0x00, 0x00, 0x00,

    /* Bulk OUT and IN endpoints. */
    0x07, 0x05, USB_CDC_OUT_EP, 0x02,
    USB_CDC_DATA_MAX_PACKET_SIZE, 0x00, 0x00,
    0x07, 0x05, USB_CDC_IN_EP, 0x02,
    USB_CDC_DATA_MAX_PACKET_SIZE, 0x00, 0x00,
};

static const uint8_t languageDescriptor[] = {
    0x04, USB_DESCRIPTOR_TYPE_STRING, 0x09, 0x04,
};

_Static_assert(sizeof(deviceDescriptor) == 18U, "Invalid USB device descriptor size");
_Static_assert(sizeof(configurationDescriptor) == 67U, "Invalid USB CDC descriptor size");
_Static_assert(USB_TX_RING_SIZE > USB_CAN_MAX_FRAME_SIZE, "USB TX ring is too small");

static uint16_t read_le16(const uint8_t *data) {
    return (uint16_t) data[0] | ((uint16_t) data[1] << 8U);
}

static void write_le16(uint8_t *data, uint16_t value) {
    data[0] = (uint8_t) value;
    data[1] = (uint8_t) (value >> 8U);
}

static void write_le32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t) value;
    data[1] = (uint8_t) (value >> 8U);
    data[2] = (uint8_t) (value >> 16U);
    data[3] = (uint8_t) (value >> 24U);
}

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFFU;

    for (uint16_t index = 0U; index < length; index++) {
        crc ^= (uint16_t) data[index] << 8U;
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x8000U) != 0U
                      ? (uint16_t) ((crc << 1U) ^ 0x1021U)
                      : (uint16_t) (crc << 1U);
        }
    }

    return crc;
}

static uint16_t make_string_descriptor(const char *text) {
    uint16_t length = 2U;

    while ((*text != '\0') && ((length + 2U) <= sizeof(usb.descriptorBuffer))) {
        usb.descriptorBuffer[length++] = (uint8_t) *text++;
        usb.descriptorBuffer[length++] = 0U;
    }

    usb.descriptorBuffer[0] = (uint8_t) length;
    usb.descriptorBuffer[1] = USB_DESCRIPTOR_TYPE_STRING;
    return length;
}

static uint16_t make_serial_descriptor(void) {
    static const char hexadecimal[] = "0123456789ABCDEF";
    const uint32_t uid[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    uint16_t output = 2U;

    for (uint8_t word = 0U; word < 3U; word++) {
        for (int8_t shift = 28; shift >= 0; shift -= 4) {
            usb.descriptorBuffer[output++] = (uint8_t) hexadecimal[(uid[word] >> shift) & 0x0FU];
            usb.descriptorBuffer[output++] = 0U;
        }
    }

    usb.descriptorBuffer[0] = (uint8_t) output;
    usb.descriptorBuffer[1] = USB_DESCRIPTOR_TYPE_STRING;
    return output;
}

static void ep0_stall(void) {
    usb.ep0State = USB_EP0_IDLE;
    usb.ep0TxRemaining = 0U;
    usb.ep0NeedsZlp = false;
    (void) HAL_PCD_EP_SetStall(&usb.handle, 0x00U);
    (void) HAL_PCD_EP_SetStall(&usb.handle, 0x80U);
}

static void ep0_status_in(void) {
    usb.ep0State = USB_EP0_STATUS_IN;
    (void) HAL_PCD_EP_Transmit(&usb.handle, 0x80U, NULL, 0U);
}

static void ep0_transmit_next(void) {
    const uint16_t packetLength = usb.ep0TxRemaining > USB_EP0_MAX_PACKET_SIZE
                                      ? USB_EP0_MAX_PACKET_SIZE
                                      : usb.ep0TxRemaining;
    uint8_t *packet = (uint8_t *) usb.ep0TxData;

    usb.ep0TxData += packetLength;
    usb.ep0TxRemaining -= packetLength;
    (void) HAL_PCD_EP_Transmit(&usb.handle, 0x80U, packet, packetLength);
}

static void ep0_send(const uint8_t *data, uint16_t length, uint16_t requestedLength) {
    if (length > requestedLength) {
        length = requestedLength;
    }

    usb.ep0State = USB_EP0_DATA_IN;
    usb.ep0NeedsZlp = (length != 0U) &&
                      ((length % USB_EP0_MAX_PACKET_SIZE) == 0U) &&
                      (length < requestedLength);
    usb.ep0TxData = data;
    usb.ep0TxRemaining = length;
    ep0_transmit_next();
}

static void set_configured(bool configured) {
    if (configured) {
        if (!usb.configured) {
            (void) HAL_PCD_EP_Open(&usb.handle, USB_CDC_IN_EP,
                                   USB_CDC_DATA_MAX_PACKET_SIZE, EP_TYPE_BULK);
            (void) HAL_PCD_EP_Open(&usb.handle, USB_CDC_OUT_EP,
                                   USB_CDC_DATA_MAX_PACKET_SIZE, EP_TYPE_BULK);
            (void) HAL_PCD_EP_Open(&usb.handle, USB_CDC_COMMAND_EP,
                                   USB_CDC_COMMAND_MAX_PACKET_SIZE, EP_TYPE_INTR);
            (void) HAL_PCD_EP_Receive(&usb.handle, USB_CDC_OUT_EP,
                                      usb.bulkOutBuffer, sizeof(usb.bulkOutBuffer));
        }
        usb.configuration = 1U;
        usb.configured = true;
    } else {
        usb.configured = false;
        usb.configuration = 0U;
        usb.txBusy = false;
        usb.flushPending = true;
        (void) HAL_PCD_EP_Close(&usb.handle, USB_CDC_IN_EP);
        (void) HAL_PCD_EP_Close(&usb.handle, USB_CDC_OUT_EP);
        (void) HAL_PCD_EP_Close(&usb.handle, USB_CDC_COMMAND_EP);
    }
}

static void handle_get_descriptor(const USB_SetupPacket *request) {
    const uint8_t descriptorType = (uint8_t) (request->wValue >> 8U);
    const uint8_t descriptorIndex = (uint8_t) request->wValue;
    const uint8_t *descriptor = NULL;
    uint16_t length = 0U;

    switch (descriptorType) {
        case USB_DESCRIPTOR_TYPE_DEVICE:
            descriptor = deviceDescriptor;
            length = sizeof(deviceDescriptor);
            break;

        case USB_DESCRIPTOR_TYPE_CONFIGURATION:
            descriptor = configurationDescriptor;
            length = sizeof(configurationDescriptor);
            break;

        case USB_DESCRIPTOR_TYPE_STRING:
            switch (descriptorIndex) {
                case 0U:
                    descriptor = languageDescriptor;
                    length = sizeof(languageDescriptor);
                    break;
                case 1U:
                    length = make_string_descriptor("Melair");
                    descriptor = usb.descriptorBuffer;
                    break;
                case 2U:
                    length = make_string_descriptor("KTANE Bomb");
                    descriptor = usb.descriptorBuffer;
                    break;
                case 3U:
                    length = make_serial_descriptor();
                    descriptor = usb.descriptorBuffer;
                    break;
                default:
                    ep0_stall();
                    return;
            }
            break;

        default:
            ep0_stall();
            return;
    }

    ep0_send(descriptor, length, request->wLength);
}

static void handle_standard_device_request(const USB_SetupPacket *request) {
    switch (request->bRequest) {
        case USB_REQUEST_GET_DESCRIPTOR:
            handle_get_descriptor(request);
            break;

        case USB_REQUEST_SET_ADDRESS:
            if ((request->wIndex != 0U) || (request->wLength != 0U) ||
                (request->wValue >= 128U) || usb.configured) {
                ep0_stall();
                break;
            }
            (void) HAL_PCD_SetAddress(&usb.handle, (uint8_t) request->wValue);
            ep0_status_in();
            break;

        case USB_REQUEST_SET_CONFIGURATION:
            if ((request->wIndex != 0U) || (request->wLength != 0U) ||
                (request->wValue > 1U)) {
                ep0_stall();
                break;
            }
            set_configured(request->wValue == 1U);
            ep0_status_in();
            break;

        case USB_REQUEST_GET_CONFIGURATION:
            usb.descriptorBuffer[0] = usb.configuration;
            ep0_send(usb.descriptorBuffer, 1U, request->wLength);
            break;

        case USB_REQUEST_GET_STATUS:
            usb.descriptorBuffer[0] = 0U;
            usb.descriptorBuffer[1] = 0U;
            ep0_send(usb.descriptorBuffer, 2U, request->wLength);
            break;

        default:
            ep0_stall();
            break;
    }
}

static void handle_standard_interface_request(const USB_SetupPacket *request) {
    if (request->wIndex > 1U) {
        ep0_stall();
        return;
    }

    switch (request->bRequest) {
        case USB_REQUEST_GET_STATUS:
            usb.descriptorBuffer[0] = 0U;
            usb.descriptorBuffer[1] = 0U;
            ep0_send(usb.descriptorBuffer, 2U, request->wLength);
            break;

        case USB_REQUEST_GET_INTERFACE:
            usb.descriptorBuffer[0] = 0U;
            ep0_send(usb.descriptorBuffer, 1U, request->wLength);
            break;

        case USB_REQUEST_SET_INTERFACE:
            if ((request->wValue == 0U) && (request->wLength == 0U)) {
                ep0_status_in();
            } else {
                ep0_stall();
            }
            break;

        default:
            ep0_stall();
            break;
    }
}

static bool endpoint_is_valid(uint8_t endpoint) {
    return (endpoint == USB_CDC_IN_EP) ||
           (endpoint == USB_CDC_OUT_EP) ||
           (endpoint == USB_CDC_COMMAND_EP);
}

static void handle_standard_endpoint_request(const USB_SetupPacket *request) {
    const uint8_t endpoint = (uint8_t) request->wIndex;

    if (!endpoint_is_valid(endpoint)) {
        ep0_stall();
        return;
    }

    switch (request->bRequest) {
        case USB_REQUEST_GET_STATUS: {
            const uint8_t number = endpoint & 0x7FU;
            const bool halted = (endpoint & 0x80U) != 0U
                                    ? usb.handle.IN_ep[number].is_stall != 0U
                                    : usb.handle.OUT_ep[number].is_stall != 0U;
            usb.descriptorBuffer[0] = halted ? 1U : 0U;
            usb.descriptorBuffer[1] = 0U;
            ep0_send(usb.descriptorBuffer, 2U, request->wLength);
            break;
        }

        case USB_REQUEST_CLEAR_FEATURE:
            if ((request->wValue != USB_FEATURE_ENDPOINT_HALT) ||
                (request->wLength != 0U)) {
                ep0_stall();
                break;
            }
            (void) HAL_PCD_EP_ClrStall(&usb.handle, endpoint);
            if (endpoint == USB_CDC_OUT_EP) {
                (void) HAL_PCD_EP_Receive(&usb.handle, USB_CDC_OUT_EP,
                                          usb.bulkOutBuffer, sizeof(usb.bulkOutBuffer));
            }
            ep0_status_in();
            break;

        case USB_REQUEST_SET_FEATURE:
            if ((request->wValue != USB_FEATURE_ENDPOINT_HALT) ||
                (request->wLength != 0U)) {
                ep0_stall();
                break;
            }
            (void) HAL_PCD_EP_SetStall(&usb.handle, endpoint);
            ep0_status_in();
            break;

        default:
            ep0_stall();
            break;
    }
}

static void handle_cdc_request(const USB_SetupPacket *request) {
    if (request->wIndex > 1U) {
        ep0_stall();
        return;
    }

    switch (request->bRequest) {
        case USB_CDC_SET_LINE_CODING:
            if (((request->bmRequest & USB_REQUEST_DIRECTION_IN) != 0U) ||
                (request->wLength != 7U)) {
                ep0_stall();
                break;
            }
            usb.pendingControlRequest = USB_CDC_SET_LINE_CODING;
            usb.ep0State = USB_EP0_DATA_OUT;
            (void) HAL_PCD_EP_Receive(&usb.handle, 0x00U, usb.lineCoding, 7U);
            break;

        case USB_CDC_GET_LINE_CODING:
            if (((request->bmRequest & USB_REQUEST_DIRECTION_IN) == 0U) ||
                (request->wLength != 7U)) {
                ep0_stall();
                break;
            }
            ep0_send(usb.lineCoding, 7U, request->wLength);
            break;

        case USB_CDC_SET_CONTROL_LINE_STATE:
            usb.controlLineState = request->wValue;
            ep0_status_in();
            break;

        case USB_CDC_SEND_BREAK:
            ep0_status_in();
            break;

        default:
            ep0_stall();
            break;
    }
}

static void handle_setup_packet(const USB_SetupPacket *request) {
    const uint8_t type = request->bmRequest & USB_REQUEST_TYPE_MASK;
    const uint8_t recipient = request->bmRequest & USB_REQUEST_RECIPIENT_MASK;

    if (type == USB_REQUEST_TYPE_CLASS) {
        handle_cdc_request(request);
        return;
    }

    if (type != USB_REQUEST_TYPE_STANDARD) {
        ep0_stall();
        return;
    }

    switch (recipient) {
        case USB_REQUEST_RECIPIENT_DEVICE:
            handle_standard_device_request(request);
            break;
        case USB_REQUEST_RECIPIENT_INTERFACE:
            handle_standard_interface_request(request);
            break;
        case USB_REQUEST_RECIPIENT_ENDPOINT:
            handle_standard_endpoint_request(request);
            break;
        default:
            ep0_stall();
            break;
    }
}

void USB_Serial_Init(void) {
    memset(&usb, 0, sizeof(usb));

    /* Default CDC line coding: 115200, 8 data bits, no parity, one stop bit. */
    write_le32(usb.lineCoding, 115200U);
    usb.lineCoding[4] = 0U;
    usb.lineCoding[5] = 0U;
    usb.lineCoding[6] = 8U;

    usb.handle.Instance = USB_DRD_FS;
    usb.handle.Init.dev_endpoints = 8U;
    usb.handle.Init.speed = PCD_SPEED_FULL;
    usb.handle.Init.phy_itface = PCD_PHY_EMBEDDED;
    usb.handle.Init.Sof_enable = DISABLE;
    usb.handle.Init.low_power_enable = DISABLE;
    usb.handle.Init.lpm_enable = DISABLE;
    usb.handle.Init.battery_charging_enable = DISABLE;
    usb.handle.Init.vbus_sensing_enable = DISABLE;
    usb.handle.Init.bulk_doublebuffer_enable = DISABLE;
    usb.handle.Init.iso_singlebuffer_enable = DISABLE;

    if (HAL_PCD_Init(&usb.handle) != HAL_OK) {
        Error_Handler();
    }

    /* PMA layout follows ST's H5 USB CDC example. */
    (void) HAL_PCDEx_PMAConfig(&usb.handle, 0x00U, PCD_SNG_BUF, 0x14U);
    (void) HAL_PCDEx_PMAConfig(&usb.handle, 0x80U, PCD_SNG_BUF, 0x54U);
    (void) HAL_PCDEx_PMAConfig(&usb.handle, USB_CDC_IN_EP, PCD_SNG_BUF, 0x94U);
    (void) HAL_PCDEx_PMAConfig(&usb.handle, USB_CDC_OUT_EP, PCD_SNG_BUF, 0xD4U);
    (void) HAL_PCDEx_PMAConfig(&usb.handle, USB_CDC_COMMAND_EP, PCD_SNG_BUF, 0x114U);

    if (HAL_PCD_Start(&usb.handle) != HAL_OK) {
        Error_Handler();
    }
}

void USB_Serial_Service(void) {
    if (usb.flushPending) {
        usb.txHead = 0U;
        usb.txTail = 0U;
        usb.flushPending = false;
    }

    if (!usb.configured || usb.txBusy || (usb.txHead == usb.txTail)) {
        return;
    }

    uint16_t length = 0U;
    while ((usb.txTail != usb.txHead) && (length < sizeof(usb.txBuffer))) {
        usb.txBuffer[length++] = usb.txRing[usb.txTail];
        usb.txTail = (uint16_t) ((usb.txTail + 1U) % USB_TX_RING_SIZE);
    }

    usb.txNeedsZlp = length == USB_CDC_DATA_MAX_PACKET_SIZE;
    usb.txBusy = true;
    if (HAL_PCD_EP_Transmit(&usb.handle, USB_CDC_IN_EP,
                            usb.txBuffer, length) != HAL_OK) {
        usb.txBusy = false;
    }
}

void USB_Serial_LogCAN(const CAN_Packet *packet) {
    if ((packet == NULL) || (packet->length > USB_CAN_MAX_PAYLOAD_SIZE) ||
        ((packet->length != 0U) && (packet->data == NULL)) || !usb.configured) {
        return;
    }

    uint8_t frame[USB_CAN_MAX_FRAME_SIZE];
    const uint16_t frameLength = USB_CAN_FRAME_OVERHEAD + packet->length;

    frame[0] = USB_SERIAL_FRAME_SYNC_0;
    frame[1] = USB_SERIAL_FRAME_SYNC_1;
    frame[2] = USB_SERIAL_FRAME_VERSION;
    frame[3] = (uint8_t) (8U + packet->length);
    write_le32(&frame[4], HAL_GetTick());
    frame[8] = (uint8_t) packet->direction;
    frame[9] = packet->length;
    write_le16(&frame[10], packet->identifier & 0x7FFU);
    memcpy(&frame[12], packet->data, packet->length);

    const uint16_t crc = crc16_ccitt(&frame[2], (uint16_t) (10U + packet->length));
    write_le16(&frame[12U + packet->length], crc);

    const uint16_t used = usb.txHead >= usb.txTail
                              ? (uint16_t) (usb.txHead - usb.txTail)
                              : (uint16_t) (USB_TX_RING_SIZE - usb.txTail + usb.txHead);
    const uint16_t free = (uint16_t) (USB_TX_RING_SIZE - used - 1U);
    if (free < frameLength) {
        usb.droppedPackets++;
        return;
    }

    for (uint16_t index = 0U; index < frameLength; index++) {
        usb.txRing[usb.txHead] = frame[index];
        usb.txHead = (uint16_t) ((usb.txHead + 1U) % USB_TX_RING_SIZE);
    }
}

uint32_t USB_Serial_GetDroppedPacketCount(void) {
    return usb.droppedPackets;
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    RCC_PeriphCLKInitTypeDef peripheralClock = {0};
    peripheralClock.PeriphClockSelection = RCC_PERIPHCLK_USB;
    peripheralClock.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&peripheralClock) != HAL_OK) {
        Error_Handler();
    }

    __HAL_RCC_CRS_CLK_ENABLE();
    RCC_CRSInitTypeDef crs = {
        .Prescaler = RCC_CRS_SYNC_DIV1,
        .Source = RCC_CRS_SYNC_SOURCE_USB,
        .Polarity = RCC_CRS_SYNC_POLARITY_RISING,
        .ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000U, 1000U),
        .ErrorLimitValue = 34U,
        .HSI48CalibrationValue = 32U,
    };
    HAL_RCCEx_CRSConfig(&crs);

    HAL_PWREx_EnableVddUSB();
    __HAL_RCC_USB_CLK_ENABLE();

    HAL_NVIC_SetPriority(USB_DRD_FS_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(USB_DRD_FS_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance == USB_DRD_FS) {
        HAL_NVIC_DisableIRQ(USB_DRD_FS_IRQn);
        __HAL_RCC_USB_CLK_DISABLE();
    }
}

void USB_DRD_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&usb.handle);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    usb.configured = false;
    usb.configuration = 0U;
    usb.txBusy = false;
    usb.flushPending = true;
    usb.ep0State = USB_EP0_IDLE;
    usb.ep0TxRemaining = 0U;
    usb.ep0NeedsZlp = false;

    (void) HAL_PCD_EP_Open(hpcd, 0x00U, USB_EP0_MAX_PACKET_SIZE, EP_TYPE_CTRL);
    (void) HAL_PCD_EP_Open(hpcd, 0x80U, USB_EP0_MAX_PACKET_SIZE, EP_TYPE_CTRL);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    const uint8_t *raw = (const uint8_t *) hpcd->Setup;
    const USB_SetupPacket request = {
        .bmRequest = raw[0],
        .bRequest = raw[1],
        .wValue = read_le16(&raw[2]),
        .wIndex = read_le16(&raw[4]),
        .wLength = read_le16(&raw[6]),
    };

    usb.pendingControlRequest = 0U;
    usb.ep0TxRemaining = 0U;
    usb.ep0NeedsZlp = false;
    handle_setup_packet(&request);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t endpoint) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    if (endpoint == 0U) {
        if ((usb.ep0State == USB_EP0_DATA_OUT) &&
            (usb.pendingControlRequest == USB_CDC_SET_LINE_CODING) &&
            (HAL_PCD_EP_GetRxCount(hpcd, 0U) == 7U)) {
            usb.pendingControlRequest = 0U;
            ep0_status_in();
        }
        return;
    }

    if (endpoint == (USB_CDC_OUT_EP & 0x7FU)) {
        (void) HAL_PCD_EP_Receive(hpcd, USB_CDC_OUT_EP,
                                  usb.bulkOutBuffer, sizeof(usb.bulkOutBuffer));
    }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t endpoint) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    if (endpoint == 0U) {
        if ((usb.ep0State == USB_EP0_DATA_IN) && (usb.ep0TxRemaining != 0U)) {
            ep0_transmit_next();
        } else if ((usb.ep0State == USB_EP0_DATA_IN) && usb.ep0NeedsZlp) {
            usb.ep0NeedsZlp = false;
            (void) HAL_PCD_EP_Transmit(hpcd, 0x80U, NULL, 0U);
        } else if (usb.ep0State == USB_EP0_DATA_IN) {
            usb.ep0State = USB_EP0_STATUS_OUT;
            (void) HAL_PCD_EP_Receive(hpcd, 0x00U, NULL, 0U);
        } else if (usb.ep0State == USB_EP0_STATUS_IN) {
            usb.ep0State = USB_EP0_IDLE;
        }
        return;
    }

    if (endpoint == (USB_CDC_IN_EP & 0x7FU)) {
        if (usb.txNeedsZlp) {
            usb.txNeedsZlp = false;
            (void) HAL_PCD_EP_Transmit(hpcd, USB_CDC_IN_EP, NULL, 0U);
        } else {
            usb.txBusy = false;
        }
    }
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance == USB_DRD_FS) {
        usb.configured = false;
        usb.txBusy = false;
        usb.flushPending = true;
    }
}
