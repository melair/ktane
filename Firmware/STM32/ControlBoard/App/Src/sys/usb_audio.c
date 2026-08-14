#include "sys/usb_audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "main.h"
#include "stm32h5xx_hal_pcd.h"
#include "stm32h5xx_it.h"

#define USB_EP0_SIZE 64u
#define USB_AUDIO_OUT_EP 0x01u
#define USB_AUDIO_FEEDBACK_EP 0x82u

#define USB_AUDIO_BYTES_PER_FRAME \
    (USB_AUDIO_CHANNEL_COUNT * (USB_AUDIO_BITS_PER_SAMPLE / 8u))
#define USB_AUDIO_NOMINAL_FRAMES_PER_MS (USB_AUDIO_SAMPLE_RATE_HZ / 1000u)
/* One extra stereo frame permits an asynchronous host to vary packet size. */
#define USB_AUDIO_OUT_PACKET_MAX \
    ((USB_AUDIO_NOMINAL_FRAMES_PER_MS + 1u) * USB_AUDIO_BYTES_PER_FRAME)

#define USB_DESC_DEVICE 0x01u
#define USB_DESC_CONFIGURATION 0x02u
#define USB_DESC_STRING 0x03u

#define USB_REQ_GET_STATUS 0x00u
#define USB_REQ_CLEAR_FEATURE 0x01u
#define USB_REQ_SET_ADDRESS 0x05u
#define USB_REQ_GET_DESCRIPTOR 0x06u
#define USB_REQ_GET_CONFIGURATION 0x08u
#define USB_REQ_SET_CONFIGURATION 0x09u
#define USB_REQ_GET_INTERFACE 0x0Au
#define USB_REQ_SET_INTERFACE 0x0Bu

#define USB_REQ_TYPE_STANDARD 0x00u
#define USB_REQ_TYPE_CLASS 0x20u
#define USB_REQ_TYPE_MASK 0x60u
#define USB_REQ_RECIPIENT_MASK 0x1Fu
#define USB_REQ_RECIPIENT_DEVICE 0x00u
#define USB_REQ_RECIPIENT_INTERFACE 0x01u
#define USB_REQ_RECIPIENT_ENDPOINT 0x02u
#define USB_REQ_DIRECTION_IN 0x80u

#define AUDIO_REQ_SET_CUR 0x01u
#define AUDIO_REQ_GET_CUR 0x81u
#define AUDIO_CONTROL_MUTE 0x01u
#define AUDIO_FEATURE_UNIT_ID 0x02u

#define USB_CONFIGURATION_VALUE 0x01u
#define USB_AUDIO_CONTROL_INTERFACE 0x00u
#define USB_AUDIO_STREAM_INTERFACE 0x01u
#define USB_AUDIO_STREAM_ALT_ACTIVE 0x01u

#define USB_CONTROL_NONE 0u
#define USB_CONTROL_MUTE_PENDING 1u

/* The H5 reserves the first 0x40 PMA bytes for all eight endpoint descriptors. */
#define USB_PMA_EP0_OUT 0x040u
#define USB_PMA_EP0_IN 0x080u
/* RX PMA sizes above 62 bytes are allocated in 32-byte blocks by the DRD.
 * The 196-byte audio maximum therefore consumes 224 bytes per buffer. */
#define USB_PMA_AUDIO_OUT_ALLOCATION ((USB_AUDIO_OUT_PACKET_MAX + 31u) & ~31u)
#define USB_PMA_AUDIO_OUT_0 0x0C0u
#define USB_PMA_AUDIO_OUT_1 (USB_PMA_AUDIO_OUT_0 + USB_PMA_AUDIO_OUT_ALLOCATION)
#define USB_PMA_FEEDBACK_IN_0 (USB_PMA_AUDIO_OUT_1 + USB_PMA_AUDIO_OUT_ALLOCATION)
#define USB_PMA_FEEDBACK_IN_1 (USB_PMA_FEEDBACK_IN_0 + 4u)

#define USB_FEEDBACK_NOMINAL (USB_AUDIO_NOMINAL_FRAMES_PER_MS << 14u)
#define USB_FEEDBACK_MAX_CORRECTION (1u << 12u)

typedef struct {
    uint8_t bm_request;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} USB_SetupPacket;

static PCD_HandleTypeDef usb_pcd = {0};
static AudioData *usb_audio = NULL;

volatile USB_AudioDiagnostics usb_audio_diagnostics = {0};

static uint8_t usb_configuration = 0u;
static uint8_t usb_stream_alt = 0u;
static uint8_t usb_control_pending = USB_CONTROL_NONE;
static uint8_t usb_mute = 0u;
static const uint8_t *usb_control_tx_next = NULL;
static uint16_t usb_control_tx_remaining = 0u;
static uint16_t usb_control_request_length = 0u;
static bool usb_control_tx_zlp_pending = false;
static bool usb_feedback_busy = false;
static bool usb_stream_started = false;
static uint32_t usb_write_frame = 0u;

static uint8_t usb_audio_packet[USB_AUDIO_OUT_PACKET_MAX] __attribute__((aligned(4)));
static uint8_t usb_feedback_packet[3] __attribute__((aligned(4)));
static uint8_t usb_control_data[USB_EP0_SIZE] __attribute__((aligned(4)));
static uint8_t usb_serial_string[26] __attribute__((aligned(4)));

static const uint8_t usb_device_descriptor[] __attribute__((aligned(4))) = {
    0x12, USB_DESC_DEVICE,
    0x00, 0x02,                         /* USB 2.0 */
    0x00, 0x00, 0x00,                  /* Class is declared per interface. */
    USB_EP0_SIZE,
    (uint8_t)(USB_AUDIO_VENDOR_ID & 0xFFu),
    (uint8_t)(USB_AUDIO_VENDOR_ID >> 8u),
    (uint8_t)(USB_AUDIO_PRODUCT_ID & 0xFFu),
    (uint8_t)(USB_AUDIO_PRODUCT_ID >> 8u),
    0x01, 0x01,                         /* Device release 1.01 */
    0x01, 0x02, 0x03,                  /* Manufacturer, product, serial */
    0x01,
};

/*
 * USB Audio Class 1.0, stereo PCM speaker with an explicit feedback endpoint.
 * The single Type-I format descriptor is what restricts the host to 48 kHz,
 * two channels and 16 bits per sample.
 */
static const uint8_t usb_configuration_descriptor[] __attribute__((aligned(4))) = {
    /* Configuration */
    0x09, USB_DESC_CONFIGURATION, 0x77, 0x00,
    0x02, USB_CONFIGURATION_VALUE, 0x00, 0x80, 0x32,

    /* Interface 0: AudioControl */
    0x09, 0x04, USB_AUDIO_CONTROL_INTERFACE, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    /* Class-specific AC header, 40 bytes of class-specific AC descriptors. */
    0x09, 0x24, 0x01, 0x00, 0x01, 0x28, 0x00, 0x01, USB_AUDIO_STREAM_INTERFACE,
    /* USB streaming input terminal, stereo left/right. */
    0x0C, 0x24, 0x02, 0x01, 0x01, 0x01, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    /* Feature unit: master mute only. */
    0x0A, 0x24, 0x06, AUDIO_FEATURE_UNIT_ID, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
    /* Speaker output terminal. */
    0x09, 0x24, 0x03, 0x03, 0x01, 0x03, 0x00, AUDIO_FEATURE_UNIT_ID, 0x00,

    /* Interface 1, alternate 0: zero bandwidth. */
    0x09, 0x04, USB_AUDIO_STREAM_INTERFACE, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    /* Interface 1, alternate 1: audio OUT plus feedback IN. */
    0x09, 0x04, USB_AUDIO_STREAM_INTERFACE, USB_AUDIO_STREAM_ALT_ACTIVE,
    0x02, 0x01, 0x02, 0x00, 0x00,
    /* Class-specific AS general descriptor. */
    0x07, 0x24, 0x01, 0x01, 0x01, 0x01, 0x00,
    /* Type-I PCM: stereo, 16-bit, one supported rate (48,000 Hz). */
    0x0B, 0x24, 0x02, 0x01, 0x02, 0x02, 0x10, 0x01, 0x80, 0xBB, 0x00,
    /* Asynchronous isochronous audio OUT endpoint, synchronized by EP 0x82. */
    0x09, 0x05, USB_AUDIO_OUT_EP, 0x05,
    (uint8_t)(USB_AUDIO_OUT_PACKET_MAX & 0xFFu),
    (uint8_t)(USB_AUDIO_OUT_PACKET_MAX >> 8u),
    0x01, 0x00, USB_AUDIO_FEEDBACK_EP,
    /* Class-specific audio data endpoint. */
    0x07, 0x25, 0x01, 0x00, 0x00, 0x00, 0x00,
    /* 10.14-format, full-speed synchronization endpoint. */
    0x09, 0x05, USB_AUDIO_FEEDBACK_EP, 0x01, 0x03, 0x00, 0x01, 0x04, 0x00,
};

static const uint8_t usb_lang_id_string[] __attribute__((aligned(4))) = {
    0x04, USB_DESC_STRING, 0x09, 0x04,
};

static const uint8_t usb_manufacturer_string[] __attribute__((aligned(4))) = {
    0x0C, USB_DESC_STRING, 'K', 0, 'T', 0, 'A', 0, 'N', 0, 'E', 0,
};

static const uint8_t usb_product_string[] __attribute__((aligned(4))) = {
    0x32, USB_DESC_STRING,
    'K', 0, 'T', 0, 'A', 0, 'N', 0, 'E', 0, ' ', 0,
    'C', 0, 'o', 0, 'n', 0, 't', 0, 'r', 0, 'o', 0, 'l', 0,
    'B', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0,
};

_Static_assert(sizeof(usb_device_descriptor) == 18u, "Invalid USB device descriptor length");
_Static_assert(sizeof(usb_configuration_descriptor) == 0x77u,
               "Invalid USB audio configuration descriptor length");
_Static_assert(sizeof(usb_product_string) == 0x32u, "Invalid USB product string length");

static uint16_t usb_min_u16(const uint16_t lhs, const uint16_t rhs) {
    return lhs < rhs ? lhs : rhs;
}

static USB_SetupPacket usb_decode_setup(const uint8_t *data) {
    USB_SetupPacket setup = {
        .bm_request = data[0],
        .request = data[1],
        .value = (uint16_t)data[2] | ((uint16_t)data[3] << 8u),
        .index = (uint16_t)data[4] | ((uint16_t)data[5] << 8u),
        .length = (uint16_t)data[6] | ((uint16_t)data[7] << 8u),
    };
    return setup;
}

static void usb_control_send(const uint8_t *data, uint16_t length) {
    const uint16_t first_packet = usb_min_u16(length, USB_EP0_SIZE);

    usb_control_tx_next = data + first_packet;
    usb_control_tx_remaining = length - first_packet;
    /* Terminate a short response that happens to end on an EP0 packet boundary. */
    usb_control_tx_zlp_pending =
        (length != 0u) && (length < usb_control_request_length) &&
        ((length % USB_EP0_SIZE) == 0u);
    (void)HAL_PCD_EP_Transmit(&usb_pcd, 0x80u, (uint8_t *)data, first_packet);
}

static void usb_control_status(void) {
    usb_control_tx_next = NULL;
    usb_control_tx_remaining = 0u;
    usb_control_tx_zlp_pending = false;
    (void)HAL_PCD_EP_Transmit(&usb_pcd, 0x80u, NULL, 0u);
}

static void usb_control_stall(void) {
    (void)HAL_PCD_EP_SetStall(&usb_pcd, 0x00u);
    (void)HAL_PCD_EP_SetStall(&usb_pcd, 0x80u);
}

static uint8_t usb_hex_digit(const uint8_t value) {
    return value < 10u ? (uint8_t)('0' + value) : (uint8_t)('A' + value - 10u);
}

static void usb_build_serial_string(void) {
    const uint64_t serial = ((uint64_t)(HAL_GetUIDw0() + HAL_GetUIDw2()) << 16u) |
                            (HAL_GetUIDw1() & 0xFFFFu);
    usb_serial_string[0] = sizeof(usb_serial_string);
    usb_serial_string[1] = USB_DESC_STRING;

    for (uint32_t digit = 0u; digit < 12u; ++digit) {
        const uint32_t shift = (11u - digit) * 4u;
        usb_serial_string[2u + digit * 2u] =
            usb_hex_digit((uint8_t)((serial >> shift) & 0x0Fu));
        usb_serial_string[3u + digit * 2u] = 0u;
    }
}

static const uint8_t *usb_get_string(const uint8_t index, uint16_t *length) {
    const uint8_t *descriptor = NULL;

    switch (index) {
        case 0u:
            descriptor = usb_lang_id_string;
            break;
        case 1u:
            descriptor = usb_manufacturer_string;
            break;
        case 2u:
            descriptor = usb_product_string;
            break;
        case 3u:
            descriptor = usb_serial_string;
            break;
        default:
            break;
    }

    if (descriptor != NULL) {
        *length = descriptor[0];
    }
    return descriptor;
}

static uint32_t usb_audio_frame_capacity(void) {
    if ((usb_audio == NULL) || (usb_audio->buffer == NULL)) {
        return 0u;
    }
    return usb_audio->buffer_size / USB_AUDIO_CHANNEL_COUNT;
}

static uint32_t usb_audio_fill_frames(void) {
    const uint32_t capacity = usb_audio_frame_capacity();
    if (capacity == 0u) {
        return 0u;
    }

    const uint32_t read_frame = I2S_GetReadFrame(usb_audio);
    return (usb_write_frame + capacity - read_frame) % capacity;
}

static void usb_audio_reset_ring(void) {
    const uint32_t capacity = usb_audio_frame_capacity();
    if (capacity == 0u) {
        usb_stream_started = false;
        usb_write_frame = 0u;
        return;
    }

    memset(usb_audio->buffer, 0, usb_audio->buffer_size * sizeof(*usb_audio->buffer));
    usb_write_frame = (I2S_GetReadFrame(usb_audio) + capacity / 2u) % capacity;
    usb_stream_started = true;
    ++usb_audio_diagnostics.ring_restarts;
}

static void usb_audio_write_packet(const uint8_t *packet, const uint16_t length) {
    const uint32_t capacity = usb_audio_frame_capacity();
    const uint32_t frame_count = length / USB_AUDIO_BYTES_PER_FRAME;

    if ((capacity == 0u) || (frame_count == 0u)) {
        return;
    }

    bool nonzero = false;
    for (uint16_t byte = 0u; byte < length; ++byte) {
        if (packet[byte] != 0u) {
            nonzero = true;
            break;
        }
    }
    if (nonzero) {
        ++usb_audio_diagnostics.out_nonzero_packets;
    }

    const uint32_t fill = usb_audio_fill_frames();
    const uint32_t guard_frames = USB_AUDIO_NOMINAL_FRAMES_PER_MS * 2u;
    if (!usb_stream_started || (fill < guard_frames) ||
        (fill > capacity - guard_frames - frame_count)) {
        usb_audio_reset_ring();
    }

    for (uint32_t frame = 0u; frame < frame_count; ++frame) {
        const uint32_t source = frame * USB_AUDIO_BYTES_PER_FRAME;
        const uint32_t destination = usb_write_frame * USB_AUDIO_CHANNEL_COUNT;
        const int16_t left = (int16_t)((uint16_t)packet[source] |
                                       ((uint16_t)packet[source + 1u] << 8u));
        const int16_t right = (int16_t)((uint16_t)packet[source + 2u] |
                                        ((uint16_t)packet[source + 3u] << 8u));

        usb_audio->buffer[destination] = usb_mute != 0u ? 0 : left;
        usb_audio->buffer[destination + 1u] = usb_mute != 0u ? 0 : right;
        usb_write_frame = (usb_write_frame + 1u) % capacity;
    }
}

static void usb_audio_endpoints_close(void) {
    if (usb_stream_alt != 0u) {
        (void)HAL_PCD_EP_Close(&usb_pcd, USB_AUDIO_OUT_EP);
        (void)HAL_PCD_EP_Close(&usb_pcd, USB_AUDIO_FEEDBACK_EP);
    }
    usb_stream_alt = 0u;
    usb_feedback_busy = false;
    usb_stream_started = false;
}

static void usb_audio_set_alternate(const uint8_t alternate) {
    if (alternate == usb_stream_alt) {
        return;
    }

    usb_audio_endpoints_close();
    if (alternate == USB_AUDIO_STREAM_ALT_ACTIVE) {
        (void)HAL_PCD_EP_Open(&usb_pcd, USB_AUDIO_OUT_EP,
                              USB_AUDIO_OUT_PACKET_MAX, EP_TYPE_ISOC);
        (void)HAL_PCD_EP_Open(&usb_pcd, USB_AUDIO_FEEDBACK_EP, 3u, EP_TYPE_ISOC);
        usb_stream_alt = alternate;
        usb_audio_reset_ring();
        (void)HAL_PCD_EP_Receive(&usb_pcd, USB_AUDIO_OUT_EP,
                                 usb_audio_packet, USB_AUDIO_OUT_PACKET_MAX);
    }
}

static bool usb_handle_standard_request(const USB_SetupPacket *setup) {
    const uint8_t recipient = setup->bm_request & USB_REQ_RECIPIENT_MASK;

    switch (setup->request) {
        case USB_REQ_GET_DESCRIPTOR: {
            const uint8_t descriptor_type = (uint8_t)(setup->value >> 8u);
            const uint8_t descriptor_index = (uint8_t)setup->value;
            const uint8_t *descriptor = NULL;
            uint16_t length = 0u;

            if (descriptor_type == USB_DESC_DEVICE) {
                descriptor = usb_device_descriptor;
                length = sizeof(usb_device_descriptor);
            } else if (descriptor_type == USB_DESC_CONFIGURATION) {
                descriptor = usb_configuration_descriptor;
                length = sizeof(usb_configuration_descriptor);
            } else if (descriptor_type == USB_DESC_STRING) {
                descriptor = usb_get_string(descriptor_index, &length);
            }

            if (descriptor == NULL) {
                return false;
            }
            usb_control_send(descriptor, usb_min_u16(length, setup->length));
            return true;
        }

        case USB_REQ_SET_ADDRESS:
            if ((recipient != USB_REQ_RECIPIENT_DEVICE) || (setup->value > 127u) ||
                (setup->length != 0u)) {
                return false;
            }
            (void)HAL_PCD_SetAddress(&usb_pcd, (uint8_t)setup->value);
            usb_control_status();
            return true;

        case USB_REQ_SET_CONFIGURATION:
            if ((recipient != USB_REQ_RECIPIENT_DEVICE) || (setup->value > 1u)) {
                return false;
            }
            usb_audio_endpoints_close();
            usb_configuration = (uint8_t)setup->value;
            usb_control_status();
            return true;

        case USB_REQ_GET_CONFIGURATION:
            if ((recipient != USB_REQ_RECIPIENT_DEVICE) || (setup->length == 0u)) {
                return false;
            }
            usb_control_data[0] = usb_configuration;
            usb_control_send(usb_control_data, 1u);
            return true;

        case USB_REQ_SET_INTERFACE:
            if ((recipient != USB_REQ_RECIPIENT_INTERFACE) ||
                (usb_configuration != USB_CONFIGURATION_VALUE) ||
                (setup->length != 0u) || (setup->index > USB_AUDIO_STREAM_INTERFACE)) {
                return false;
            }

            if (setup->index == USB_AUDIO_CONTROL_INTERFACE) {
                if (setup->value != 0u) {
                    return false;
                }
            } else {
                if (setup->value > USB_AUDIO_STREAM_ALT_ACTIVE) {
                    return false;
                }
                usb_audio_set_alternate((uint8_t)setup->value);
            }
            usb_control_status();
            return true;

        case USB_REQ_GET_INTERFACE:
            if ((recipient != USB_REQ_RECIPIENT_INTERFACE) ||
                (usb_configuration != USB_CONFIGURATION_VALUE) ||
                (setup->index > USB_AUDIO_STREAM_INTERFACE)) {
                return false;
            }
            usb_control_data[0] = setup->index == USB_AUDIO_STREAM_INTERFACE
                                      ? usb_stream_alt
                                      : 0u;
            usb_control_send(usb_control_data, 1u);
            return true;

        case USB_REQ_GET_STATUS:
            if (setup->length != 2u) {
                return false;
            }
            usb_control_data[0] = 0u;
            usb_control_data[1] = 0u;
            usb_control_send(usb_control_data, 2u);
            return true;

        case USB_REQ_CLEAR_FEATURE:
            if ((recipient == USB_REQ_RECIPIENT_ENDPOINT) && (setup->value == 0u)) {
                (void)HAL_PCD_EP_ClrStall(&usb_pcd, (uint8_t)setup->index);
                usb_control_status();
                return true;
            }
            return false;

        default:
            return false;
    }
}

static bool usb_handle_audio_request(const USB_SetupPacket *setup) {
    const uint8_t control_selector = (uint8_t)(setup->value >> 8u);
    const uint8_t entity = (uint8_t)(setup->index >> 8u);
    const uint8_t interface = (uint8_t)setup->index;

    if (((setup->bm_request & USB_REQ_RECIPIENT_MASK) != USB_REQ_RECIPIENT_INTERFACE) ||
        (control_selector != AUDIO_CONTROL_MUTE) ||
        (entity != AUDIO_FEATURE_UNIT_ID) ||
        (interface != USB_AUDIO_CONTROL_INTERFACE) || (setup->length != 1u)) {
        return false;
    }

    if ((setup->bm_request & USB_REQ_DIRECTION_IN) != 0u) {
        if (setup->request != AUDIO_REQ_GET_CUR) {
            return false;
        }
        usb_control_data[0] = usb_mute;
        usb_control_send(usb_control_data, 1u);
        return true;
    }

    if (setup->request != AUDIO_REQ_SET_CUR) {
        return false;
    }
    usb_control_pending = USB_CONTROL_MUTE_PENDING;
    (void)HAL_PCD_EP_Receive(&usb_pcd, 0x00u, usb_control_data, 1u);
    return true;
}

void USB_Audio_Init(void) {
    usb_build_serial_string();
    usb_audio_diagnostics = (USB_AudioDiagnostics){0};

    usb_pcd.Instance = USB_DRD_FS;
    usb_pcd.Init.dev_endpoints = 3u;
    usb_pcd.Init.speed = PCD_SPEED_FULL;
    usb_pcd.Init.ep0_mps = PCD_EP0MPS_64;
    usb_pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
    usb_pcd.Init.Sof_enable = ENABLE;
    usb_pcd.Init.low_power_enable = DISABLE;
    usb_pcd.Init.lpm_enable = DISABLE;
    usb_pcd.Init.battery_charging_enable = DISABLE;
    usb_pcd.Init.vbus_sensing_enable = DISABLE;
    usb_pcd.Init.bulk_doublebuffer_enable = DISABLE;
    usb_pcd.Init.iso_singlebuffer_enable = DISABLE;

    if (HAL_PCD_Init(&usb_pcd) != HAL_OK) {
        Error_Handler();
    }

    (void)HAL_PCDEx_PMAConfig(&usb_pcd, 0x00u, PCD_SNG_BUF, USB_PMA_EP0_OUT);
    (void)HAL_PCDEx_PMAConfig(&usb_pcd, 0x80u, PCD_SNG_BUF, USB_PMA_EP0_IN);
    (void)HAL_PCDEx_PMAConfig(&usb_pcd, USB_AUDIO_OUT_EP, PCD_DBL_BUF,
                              ((uint32_t)USB_PMA_AUDIO_OUT_1 << 16u) |
                                  USB_PMA_AUDIO_OUT_0);
    (void)HAL_PCDEx_PMAConfig(&usb_pcd, USB_AUDIO_FEEDBACK_EP, PCD_DBL_BUF,
                              ((uint32_t)USB_PMA_FEEDBACK_IN_1 << 16u) |
                                  USB_PMA_FEEDBACK_IN_0);

    if (HAL_PCD_Start(&usb_pcd) != HAL_OK) {
        Error_Handler();
    }
}

void USB_Audio_Attach(AudioData *audio) {
    const uint32_t interrupt_state = __get_PRIMASK();
    __disable_irq();
    usb_audio = audio;
    usb_stream_started = false;
    if (usb_stream_alt == USB_AUDIO_STREAM_ALT_ACTIVE) {
        usb_audio_reset_ring();
    }
    if (interrupt_state == 0u) {
        __enable_irq();
    }
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance != USB_DRD_FS) {
        return;
    }

    RCC_PeriphCLKInitTypeDef peripheral_clock = {
        .PeriphClockSelection = RCC_PERIPHCLK_USB,
        .UsbClockSelection = RCC_USBCLKSOURCE_HSI48,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&peripheral_clock) != HAL_OK) {
        Error_Handler();
    }

    __HAL_RCC_CRS_CLK_ENABLE();
    RCC_CRSInitTypeDef crs = {
        .Prescaler = RCC_CRS_SYNC_DIV1,
        .Source = RCC_CRS_SYNC_SOURCE_USB,
        .Polarity = RCC_CRS_SYNC_POLARITY_RISING,
        .ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000u, 1000u),
        .ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT,
        .HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT,
    };
    HAL_RCCEx_CRSConfig(&crs);

    HAL_PWREx_EnableVddUSB();
    __HAL_RCC_USB_CLK_ENABLE();

    HAL_NVIC_SetPriority(USB_DRD_FS_IRQn, 1u, 0u);
    HAL_NVIC_EnableIRQ(USB_DRD_FS_IRQn);
}

void USB_DRD_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&usb_pcd);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd != &usb_pcd) {
        return;
    }

    usb_audio_endpoints_close();
    usb_configuration = 0u;
    usb_control_pending = USB_CONTROL_NONE;
    usb_control_tx_next = NULL;
    usb_control_tx_remaining = 0u;
    usb_control_request_length = 0u;
    usb_control_tx_zlp_pending = false;
    usb_mute = 0u;

    (void)HAL_PCD_EP_Open(hpcd, 0x00u, USB_EP0_SIZE, EP_TYPE_CTRL);
    (void)HAL_PCD_EP_Open(hpcd, 0x80u, USB_EP0_SIZE, EP_TYPE_CTRL);
    (void)HAL_PCD_EP_Receive(hpcd, 0x00u, NULL, 0u);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    if (hpcd != &usb_pcd) {
        return;
    }

    usb_control_pending = USB_CONTROL_NONE;
    const USB_SetupPacket setup = usb_decode_setup((const uint8_t *)hpcd->Setup);
    usb_control_tx_next = NULL;
    usb_control_tx_remaining = 0u;
    usb_control_request_length = setup.length;
    usb_control_tx_zlp_pending = false;
    bool handled = false;

    if ((setup.bm_request & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD) {
        handled = usb_handle_standard_request(&setup);
    } else if ((setup.bm_request & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS) {
        handled = usb_handle_audio_request(&setup);
    }

    if (!handled) {
        usb_control_stall();
    }
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t endpoint) {
    if (hpcd != &usb_pcd) {
        return;
    }

    if (endpoint == 0u) {
        if (usb_control_pending == USB_CONTROL_MUTE_PENDING) {
            usb_mute = usb_control_data[0] != 0u ? 1u : 0u;
            usb_control_pending = USB_CONTROL_NONE;
            usb_audio_reset_ring();
            usb_control_status();
        }
        return;
    }

    if ((endpoint == USB_AUDIO_OUT_EP) &&
        (usb_stream_alt == USB_AUDIO_STREAM_ALT_ACTIVE)) {
        const uint16_t received = (uint16_t)HAL_PCD_EP_GetRxCount(hpcd, endpoint);
        ++usb_audio_diagnostics.out_packets;
        usb_audio_diagnostics.out_bytes += received;
        usb_audio_diagnostics.last_out_length = received;
        if ((received <= USB_AUDIO_OUT_PACKET_MAX) &&
            ((received % USB_AUDIO_BYTES_PER_FRAME) == 0u)) {
            usb_audio_write_packet(usb_audio_packet, received);
        } else {
            ++usb_audio_diagnostics.out_bad_packets;
        }
        (void)HAL_PCD_EP_Receive(hpcd, USB_AUDIO_OUT_EP,
                                 usb_audio_packet, USB_AUDIO_OUT_PACKET_MAX);
    }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t endpoint) {
    if (hpcd != &usb_pcd) {
        return;
    }

    if (endpoint == 0u) {
        if (usb_control_tx_remaining != 0u) {
            const uint16_t packet_length =
                usb_min_u16(usb_control_tx_remaining, USB_EP0_SIZE);
            const uint8_t *packet = usb_control_tx_next;
            usb_control_tx_next += packet_length;
            usb_control_tx_remaining -= packet_length;
            (void)HAL_PCD_EP_Transmit(hpcd, 0x80u, (uint8_t *)packet,
                                      packet_length);
            return;
        }

        if (usb_control_tx_zlp_pending) {
            usb_control_tx_zlp_pending = false;
            (void)HAL_PCD_EP_Transmit(hpcd, 0x80u, NULL, 0u);
            return;
        }

        /* Accept either the OUT status stage or the next SETUP packet. */
        (void)HAL_PCD_EP_Receive(hpcd, 0x00u, NULL, 0u);
    } else if (endpoint == (USB_AUDIO_FEEDBACK_EP & 0x7Fu)) {
        usb_feedback_busy = false;
        ++usb_audio_diagnostics.feedback_completions;
    }
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
    if ((hpcd != &usb_pcd) || (usb_stream_alt != USB_AUDIO_STREAM_ALT_ACTIVE) ||
        usb_feedback_busy) {
        return;
    }

    int32_t correction = 0;
    const uint32_t capacity = usb_audio_frame_capacity();
    if ((capacity != 0u) && usb_stream_started) {
        const uint32_t guard_frames = USB_AUDIO_NOMINAL_FRAMES_PER_MS * 2u;
        const uint32_t fill = usb_audio_fill_frames();
        if ((fill < guard_frames) || (fill > capacity - guard_frames)) {
            usb_audio_reset_ring();
        }
        const int32_t target = (int32_t)(capacity / 2u);
        const int32_t error = (int32_t)usb_audio_fill_frames() - target;
        correction = -(error * 8);
        if (correction > (int32_t)USB_FEEDBACK_MAX_CORRECTION) {
            correction = USB_FEEDBACK_MAX_CORRECTION;
        } else if (correction < -(int32_t)USB_FEEDBACK_MAX_CORRECTION) {
            correction = -(int32_t)USB_FEEDBACK_MAX_CORRECTION;
        }
    }

    const uint32_t feedback = (uint32_t)((int32_t)USB_FEEDBACK_NOMINAL + correction);
    usb_feedback_packet[0] = (uint8_t)feedback;
    usb_feedback_packet[1] = (uint8_t)(feedback >> 8u);
    usb_feedback_packet[2] = (uint8_t)(feedback >> 16u);
    usb_feedback_busy = true;
    usb_audio_diagnostics.last_feedback_10_14 = feedback;
    if (HAL_PCD_EP_Transmit(hpcd, USB_AUDIO_FEEDBACK_EP,
                            usb_feedback_packet, sizeof(usb_feedback_packet)) != HAL_OK) {
        usb_feedback_busy = false;
    } else {
        ++usb_audio_diagnostics.feedback_packets;
    }
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t endpoint) {
    if ((hpcd == &usb_pcd) && (endpoint == USB_AUDIO_OUT_EP) &&
        (usb_stream_alt == USB_AUDIO_STREAM_ALT_ACTIVE)) {
        (void)HAL_PCD_EP_Receive(hpcd, USB_AUDIO_OUT_EP,
                                 usb_audio_packet, USB_AUDIO_OUT_PACKET_MAX);
    }
}
