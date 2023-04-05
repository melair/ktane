#ifndef PROTOCOL_H
#define	PROTOCOL_H

void protocol_receive(uint8_t prefix, uint8_t id, uint8_t size, uint8_t *payload);

#define PREFIX_MODULE       0b000
#define PREFIX_GAME         0b010

#define PREFIX_NETWORK      0b100

#define PREFIX_FIRMWARE     0b110
#define PREFIX_DEBUG        0b111

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
            
            struct {
                uint8_t domain;
            } domain_announce;
        } network;
        
        union {
            struct {
                uint8_t mode;
                uint16_t application_version;
                struct {
                    unsigned ready  :1;
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
                uint8_t strikes;
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

#endif	/* PROTOCOL_H */

