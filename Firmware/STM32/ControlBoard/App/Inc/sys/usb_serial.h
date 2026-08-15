#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "sys/can.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CAN record wire format (all multi-byte values are little-endian):
 *
 *   Offset  Size  Field
 *      0      2   Sync: 0xA5, 0x5A
 *      2      1   Format version (1)
 *      3      1   Body length (8 + CAN payload length)
 *      4      4   HAL_GetTick() in milliseconds
 *      8      1   CAN_Direction (0 = in, 1 = out)
 *      9      1   CAN payload length
 *     10      2   11-bit CAN identifier
 *     12      N   CAN payload
 *   12 + N    2   CRC-16/CCITT-FALSE over version through payload
 *
 * A complete record is therefore 14 + N bytes long.
 */
#define USB_SERIAL_FRAME_SYNC_0 0xA5U
#define USB_SERIAL_FRAME_SYNC_1 0x5AU
#define USB_SERIAL_FRAME_VERSION 1U

void USB_Serial_Init(void);

void USB_Serial_Service(void);

void USB_Serial_LogCAN(const CAN_Packet *packet);

uint32_t USB_Serial_GetDroppedPacketCount(void);

#ifdef __cplusplus
}
#endif

#endif //USB_SERIAL_H
