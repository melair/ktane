#include "cobs/cobs.h"

#include <string.h>

static bool decode(COBS_State *cobs, size_t *decoded_length) {
    size_t read_index = 0U;
    size_t write_index = 0U;

    while (read_index < cobs->rx_length) {
        const uint8_t code = cobs->rx_buffer[read_index++];

        if (code == 0U) {
            return false;
        }

        const size_t copy_length = (size_t) code - 1U;

        if ((copy_length > (cobs->rx_length - read_index)) ||
            (copy_length > (COBS_PACKET_MAX_SIZE - write_index))) {
            return false;
        }

        memcpy(&cobs->decoded_buffer[write_index], &cobs->rx_buffer[read_index], copy_length);
        read_index += copy_length;
        write_index += copy_length;

        if ((code != 0xFFU) && (read_index < cobs->rx_length)) {
            if (write_index >= COBS_PACKET_MAX_SIZE) {
                return false;
            }

            cobs->decoded_buffer[write_index++] = 0U;
        }
    }

    *decoded_length = write_index;
    return true;
}

static void process_byte(COBS_State *cobs, uint8_t value) {
    if (!cobs->rx_synced) {
        cobs->rx_synced = value == 0U;
        return;
    }

    if (value == 0U) {
        if ((cobs->rx_length > 0U) && (cobs->packet_handler != NULL)) {
            size_t decoded_length;

            if (decode(cobs, &decoded_length)) {
                cobs->packet_handler(cobs->decoded_buffer, decoded_length);
            }
        }

        cobs->rx_length = 0U;
        return;
    }

    if (cobs->rx_length >= COBS_ENCODED_MAX_SIZE) {
        cobs->rx_length = 0U;
        cobs->rx_synced = false;
        return;
    }

    cobs->rx_buffer[cobs->rx_length++] = value;
}

void COBS_Init(COBS_State *cobs, COBS_PacketHandler packet_handler) {
    if (cobs == NULL) {
        return;
    }

    memset(cobs, 0, sizeof(*cobs));
    cobs->packet_handler = packet_handler;
}

void COBS_Service(COBS_State *cobs, const uint8_t *data, size_t length) {
    if ((cobs == NULL) || ((data == NULL) && (length > 0U))) {
        return;
    }

    for (size_t index = 0U; index < length; index++) {
        process_byte(cobs, data[index]);
    }
}

bool COBS_Encode(const uint8_t *data, size_t length,
                 uint8_t *frame, size_t frame_capacity, size_t *frame_length) {
    const size_t required_capacity = length + (length / 254U) + 2U;

    if (((data == NULL) && (length > 0U)) ||
        (length > COBS_PACKET_MAX_SIZE) ||
        (frame == NULL) ||
        (frame_length == NULL) ||
        (frame_capacity < required_capacity)) {
        return false;
    }

    uint8_t *const start = frame;
    uint8_t *code_pointer = frame++;
    uint8_t code = 1U;

    for (size_t index = 0U; index < length; index++) {
        if (data[index] == 0U) {
            *code_pointer = code;
            code_pointer = frame++;
            code = 1U;
        } else {
            *frame++ = data[index];
            code++;

            if (code == 0xFFU) {
                *code_pointer = code;
                code_pointer = frame++;
                code = 1U;
            }
        }
    }

    *code_pointer = code;
    *frame++ = 0U;
    *frame_length = (size_t) (frame - start);
    return true;
}
