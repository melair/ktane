#ifndef PROTOCOL_H
#define	PROTOCOL_H

#define PREFIX_MODULE       0x00
#define PREFIX_GAME         0x02
#define PREFIX_NETWORK      0x04
#define PREFIX_FIRMWARE     0x06
#define PREFIX_DEBUG        0x07 

#define OPCODE_NETWORK_ADDRESS_ANNOUNCE 0x00
#define OPCODE_NETWORK_ADDRESS_NAK      0x01

#define OPCODE_MODULE_ANNOUNCEMENT      0x00
#define OPCODE_MODULE_RESET             0x10
#define OPCODE_MODULE_IDENTIFY          0x11
#define OPCODE_MODULE_MODE_SET          0x12
#define OPCODE_MODULE_SPECIAL_FUNCTION  0x13
#define OPCODE_MODULE_ERROR             0xf0

#define OPCODE_GAME_STATE           0x00
#define OPCODE_GAME_MODULE_CONFIG   0x10
#define OPCODE_GAME_MODULE_STATE    0x11
#define OPCODE_GAME_MODULE_STRIKE   0x12

#define OPCODE_FIRMWARE_REQUEST         0x00
#define OPCODE_FIRMWARE_HEADER          0x01
#define OPCODE_FIRMWARE_PAGE_REQUEST    0x02
#define OPCODE_FIRMWARE_PAGE_RESPONSE   0x03

#define SIZE_NETWORK_ADDRESS_ANNOUNCE sizeof(((packet_t *)0)->network.address_announce) + 1
#define SIZE_NETWORK_ADDRESS_NAK      sizeof(((packet_t *)0)->network.address_nak) + 1

#define SIZE_MODULE_ANNOUNCEMENT      sizeof(((packet_t *)0)->module.announcement) + 1
#define SIZE_MODULE_RESET             sizeof(((packet_t *)0)->module.reset) + 1
#define SIZE_MODULE_IDENTIFY          sizeof(((packet_t *)0)->module.identify) + 1
#define SIZE_MODULE_MODE_SET          sizeof(((packet_t *)0)->module.set_mode) + 1
#define SIZE_MODULE_SPECIAL_FUNCTION  sizeof(((packet_t *)0)->module.special_function) + 1
#define SIZE_MODULE_ERROR             sizeof(((packet_t *)0)->module.error_announcement) + 1

#define SIZE_GAME_STATE               sizeof(((packet_t *)0)->game.state) + 1
#define SIZE_GAME_MODULE_CONFIG       sizeof(((packet_t *)0)->game.module_config) + 1
#define SIZE_GAME_MODULE_STATE        sizeof(((packet_t *)0)->game.module_state) + 1
#define SIZE_GAME_MODULE_STRIKE       sizeof(((packet_t *)0)->game.module_strike) + 1

#define SIZE_FIRMWARE_REQUEST         sizeof(((packet_t *)0)->firmware.request) + 1
#define SIZE_FIRMWARE_HEADER          sizeof(((packet_t *)0)->firmware.header) + 1
#define SIZE_FIRMWARE_PAGE_REQUEST    sizeof(((packet_t *)0)->firmware.page_request) + 1
#define SIZE_FIRMWARE_PAGE_RESPONSE   sizeof(((packet_t *)0)->firmware.page_response) + 1

#include <stdint.h>

typedef struct {
    uint8_t opcode;
    
    union {        
        union {
            struct {
                uint32_t serial;
            } address_announce;
            
            struct {
                uint32_t serial;
            } address_nak;
        } network;
        
        union {
            struct {
                uint8_t mode;
                uint16_t application_version;
                struct {
                    unsigned reset  :1;
                    unsigned debug  :1;
                    unsigned unused :6;
                } flags;
                uint32_t serial;
                uint16_t bootloader_version;
                uint16_t flasher_version;
            } announcement;
            
            struct {
                uint16_t error_code;
                struct {
                    unsigned active :1;
                    unsigned unused :7;
                } flags;
            } error_announcement;
            
            struct {
            } reset;
            
            struct {
                uint8_t can_id;
            } identify;
            
            struct {
                uint8_t can_id;
                uint8_t mode;
            } set_mode;
            
            struct {
                uint8_t can_id;
                uint8_t special_function;
            } special_function;                        
        } module;
        
        union {
            struct {
                uint8_t state;
                uint32_t seed;
                uint8_t strikes_current;
                uint8_t strikes_total;
                uint8_t minutes;
                uint8_t seconds;
                uint8_t centiseconds;
                uint8_t time_ratio;                        
            } state;
            
            struct {
                uint8_t can_id;
                struct {
                    unsigned enabled :1;
                    unsigned unused :7;
                } flags;
                uint8_t difficulty;
            } module_config;
            
            struct {
                struct {
                    unsigned ready :1;
                    unsigned solved :1;
                    unsigned unused :6;
                } flags;
            } module_state;
            
            struct {
                uint8_t strikes;
            } module_strike;
        } game;
       
        union {
            struct {
                uint16_t version;
                uint8_t segment;
            } request;
            
            struct {
                uint16_t version;
                uint16_t pages;
                uint32_t crc;
                uint8_t segment;
            } header;
            
            struct {
                uint16_t page;
                uint8_t source_id;
                uint8_t segment;
            } page_request;
            
            struct {
                uint16_t page;
                uint8_t segment;
                uint8_t data[16];
            } page_response;            
        } firmware;
    };
} packet_t;

typedef struct {
    uint8_t prefix;
    uint8_t opcode;
    uint8_t size;
} protocol_size_t;

#endif	/* PROTOCOL_H */

