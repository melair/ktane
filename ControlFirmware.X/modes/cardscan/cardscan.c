#include <xc.h>
#include <stdbool.h>
#include "cardscan.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../spi.h"
#include "../../tick.h"
#include "../../peripherals/lcd.h"

#define CARDSCAN_RNG_MASK 0x96ea865c

void cardscan_service(void);
void pn532_service(void);
void pn532_service_samconfigure_callback(bool ok);
void pn532_service_inpassivetarget_callback(bool ok);
void pn532_service_detect_callback(bool ok);
void pn532_service_mfu_read_callback(bool ok);
void pn532_service_mfu_write_callback(bool ok);

void pn532_spi_service(void);
void pn532_spi_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(bool));
spi_command_t *pn532_spi_callback_ack(spi_command_t *cmd);
spi_command_t *pn532_spi_callback_response(spi_command_t *cmd);
spi_command_t *pn532_spi_callback_write(spi_command_t *cmd);

uint8_t *pn532_packet_prepare(uint8_t *buff, uint8_t cmd_len);
void pn532_packet_finalise(uint8_t *buff);
uint8_t pn532_size(uint8_t cmd_len);

uint8_t pn532_getfirmwareversion(uint8_t *buff);
uint8_t pn532_samconfigure(uint8_t *buff);
uint8_t pn532_inlistpassivetarget(uint8_t *buff);
uint8_t pn532_statread(uint8_t *buff);
uint8_t pn532_dataread(uint8_t *buff);
uint8_t pn532_mifareultralight_write(uint8_t *buff, uint8_t page, uint8_t *data);
uint8_t pn532_mifareultralight_read(uint8_t *buff, uint8_t page);
        
#define CARDSCAN_SIZE 16
bool cardscan_prog = false;
uint8_t cardscan_prog_id = 0;

const spi_device_t cardscan_device = {
    .clk_pin = KPIN_C0,
    .miso_pin = KPIN_C1,
    .mosi_pin = KPIN_C2,
    .cs_pin = KPIN_C3,
    .baud = SPI_BAUD_125_KHZ,
    .lsb_first = 1,
};

void cardscan_initialise(void) {
    /* Initialise the LCD. */
    lcd_initialize();
    
    /* Initialise SPI, register RFID. */
    kpin_mode(KPIN_C0, PIN_OUTPUT, false);
    kpin_mode(KPIN_C1, PIN_INPUT, false);
    kpin_mode(KPIN_C2, PIN_OUTPUT, false);
    kpin_mode(KPIN_C3, PIN_OUTPUT, false);    
        
    /* Set CS to high. */
    kpin_write(KPIN_C3, true);
    
    /* Initialise SPI. */
    mode_data.cardscan.spi_device = spi_register(&cardscan_device);
    
    /* Initialise IRQ. */
    kpin_mode(KPIN_C4, PIN_INPUT, false);
    
    /* Register callbacks. */
    mode_register_callback(GAME_ALWAYS, cardscan_service, NULL);

    mode_data.cardscan.pn532_wait_time = tick_value;
}

void cardscan_service(void) {
     lcd_service();   
     pn532_spi_service();
     pn532_service();
}

#define PN532_STATE_POWER_ON_WAIT        0
#define PN532_STATE_CONFIG               1
#define PN532_STATE_CONFIG_WAIT          2
#define PN532_STATE_DETECT_START         3
#define PN532_STATE_DETECT_START_ACK     4
#define PN532_STATE_DETECT_WAIT          5
#define PN532_STATE_CARD_FOUND_INFLIGHT  6
#define PN532_STATE_BLOCK_READ           7
#define PN532_STATE_BLOCK_READ_INFLIGHT  8
#define PN532_STATE_BLOCK_WRITE          9
#define PN532_STATE_BLOCK_WRITE_INFLIGHT 10
#define PN532_STATE_COOLDOWN             11
#define PN532_STATE_COOLDOWN_WAIT        12


#define PN532_POWER_ON_WAIT 500
#define PN532_COOLDOWN      250

void pn532_service(void) {  
    uint8_t size;
    uint8_t writebuf[4] = { 0xc0, 0xff, 0xee, 0x00 };

    switch(mode_data.cardscan.pn532_state) {
        case PN532_STATE_POWER_ON_WAIT:
            if (mode_data.cardscan.pn532_wait_time + PN532_POWER_ON_WAIT < tick_value) {
                mode_data.cardscan.pn532_state = PN532_STATE_CONFIG;
            }

            break;
            
        case PN532_STATE_CONFIG:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_CONFIG_WAIT;
            size = pn532_samconfigure(&mode_data.cardscan.spi_buffer);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 9, &pn532_service_samconfigure_callback);         
            break;
            
        case PN532_STATE_CONFIG_WAIT:
            // TODO: Jump back if config times out.
            break;
            
        case PN532_STATE_DETECT_START:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START_ACK;
            size = pn532_inlistpassivetarget(&mode_data.cardscan.spi_buffer);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 0, &pn532_service_inpassivetarget_callback);  
            break;
            
        case PN532_STATE_DETECT_START_ACK:
            break;
           
        case PN532_STATE_DETECT_WAIT:
            if (!kpin_read(KPIN_C4)) {
                mode_data.cardscan.pn532_state = PN532_STATE_CARD_FOUND_INFLIGHT;
                pn532_spi_send(&mode_data.cardscan.spi_buffer[0], 0, 22, &pn532_service_detect_callback);  
            }
            break;

        case PN532_STATE_CARD_FOUND_INFLIGHT:
            break;
            
        case PN532_STATE_BLOCK_READ:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_READ_INFLIGHT;
            size = pn532_mifareultralight_read(&mode_data.cardscan.spi_buffer, 4);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 26, &pn532_service_mfu_read_callback);              
            break;
            
        case PN532_STATE_BLOCK_READ_INFLIGHT:
            break;
            
        case PN532_STATE_BLOCK_WRITE:
            writebuf[3] = cardscan_prog_id;
            
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_WRITE_INFLIGHT;
            size = pn532_mifareultralight_write(&mode_data.cardscan.spi_buffer, 4, &writebuf);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 10, &pn532_service_mfu_write_callback); 
            break;
            
        case PN532_STATE_BLOCK_WRITE_INFLIGHT:
            break;
            
        case PN532_STATE_COOLDOWN:
            mode_data.cardscan.pn532_wait_time = tick_value;
            mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN_WAIT;
            break;
            
        case PN532_STATE_COOLDOWN_WAIT:
            if (mode_data.cardscan.pn532_wait_time + PN532_COOLDOWN < tick_value) {
                mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START;
            }
            break;
    }        
}

/*
 * d5 = pn532 to host
 * 4b = response to in passive target
 * 01 = number of cards
 * 01 = card 1
 * 00 44 = ATQA (0x00 0x44 == Mifare Ultralight)
 * 00 = SAK (0x00 == Mifare Ultralight)
 * 07 = UID Len (0x07 == Mifare Ultralight)
 * XX(0-6) = UID
 * 
 * We can validate ATQA, SAK AND UID Len to verify if we want to talk to card.
 * 
 * Store data in 4 bytes at block 4, first user slot. Either we store the
 * card number, or we store the facts. Card number is probably simpler.
 */

void pn532_service_inpassivetarget_callback(bool ok) {
    if (ok) {
        mode_data.cardscan.pn532_state = PN532_STATE_DETECT_WAIT;   
    } else {
        
    }
}

void pn532_service_samconfigure_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.spi_buffer[6] == 0x15) {
            mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START;
        } else {
            // Error condition.
        }       
    } else {
        
    }
}

void pn532_service_detect_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x4b && mode_data.cardscan.spi_buffer[7] == 0x01) {
            uint16_t atqa = (mode_data.cardscan.spi_buffer[9] << 8) | mode_data.cardscan.spi_buffer[10];
            uint8_t sak = mode_data.cardscan.spi_buffer[11];
            uint8_t uid_len = mode_data.cardscan.spi_buffer[12];

            /* Verify if a Mifare Ultralight. */
            if (atqa == 0x0044 && sak == 0x00 && uid_len == 0x07) {                
                if (cardscan_prog) {
                    mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_WRITE;
                } else {
                    mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_READ;
                }
            } else {
                mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
            }
        } else {
            mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
        }
    }
}

void pn532_service_mfu_read_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x41) {
            if (mode_data.cardscan.spi_buffer[7] == 0x00 &&
                mode_data.cardscan.spi_buffer[8] == 0xc0 &&
                mode_data.cardscan.spi_buffer[9] == 0xff &&
                mode_data.cardscan.spi_buffer[10] == 0xee) {

                // Read Success
                uint8_t card_id = mode_data.cardscan.spi_buffer[10];
            } else {
            }
        } else {
            
        }
    } else {
    }
    
    mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
}

void pn532_service_mfu_write_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x41) {
            if (mode_data.cardscan.spi_buffer[7] == 0x00 && (mode_data.cardscan.spi_buffer[8] & 0x0f) == 0x0a) {
                // Write Success!
                cardscan_prog_id++;
                
                if (cardscan_prog_id == CARDSCAN_SIZE) {
                    cardscan_prog = false;
                }
            } else {
            }
        } else {
            
        }
    } else {
    }
    
    mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
}

uint8_t pn532_cmd_state;
uint8_t pn532_cmd_write_size;
uint8_t pn532_cmd_read_size;
uint8_t *pn532_cmd_buffer;
void (*pn532_cmd_callback)(bool);

#define PN532_CMD_STATE_IDLE                    0
#define PN532_CMD_STATE_WRITE                   1
#define PN532_CMD_STATE_WRITE_IN_FLIGHT         2
#define PN532_CMD_STATE_WAIT_FOR_ACK_IRQ        3
#define PN352_CMD_STATE_READ_ACK                4
#define PN352_CMD_STATE_READ_ACK_IN_FLIGHT      5
#define PN532_CMD_STATE_WAIT_FOR_RESPONSE_IRQ   6
#define PN532_CMD_STATE_READ_RESPONSE           7
#define PN532_CMD_STATE_READ_RESPONSE_IN_FLIGHT 8
#define PN532_CMD_STATE_DONE                    9

void pn532_spi_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(bool)) {
    if (write_size > 0) {
        pn532_cmd_state = PN532_CMD_STATE_WRITE;
    } else {
        pn532_cmd_state = PN532_CMD_STATE_READ_RESPONSE;
    }
    pn532_cmd_buffer = buff;
    pn532_cmd_write_size = write_size;
    pn532_cmd_read_size = read_size;
    pn532_cmd_callback = callback;
    
    pn532_spi_service();
}

void pn532_spi_service(void) {   
    uint8_t size = 0;
    
    switch(pn532_cmd_state) {
        case PN532_CMD_STATE_IDLE:
            // No Action
            break;
            
        case PN532_CMD_STATE_WRITE:           
            mode_data.cardscan.pn532_cmd.device = mode_data.cardscan.spi_device;
            mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE;
            mode_data.cardscan.pn532_cmd.buffer = pn532_cmd_buffer;
            mode_data.cardscan.pn532_cmd.write_size = pn532_cmd_write_size;
            mode_data.cardscan.pn532_cmd.read_size = 0;
            mode_data.cardscan.pn532_cmd.callback = &pn532_spi_callback_write;
            
            spi_enqueue(&mode_data.cardscan.pn532_cmd);
            pn532_cmd_state = PN532_CMD_STATE_WRITE_IN_FLIGHT;
            break;
            
        case PN532_CMD_STATE_WRITE_IN_FLIGHT:
            break;
            
        case PN532_CMD_STATE_WAIT_FOR_ACK_IRQ:
            if (!kpin_read(KPIN_C4)) {
                pn532_cmd_state = PN352_CMD_STATE_READ_ACK;
            }
            break;
                       
        case PN352_CMD_STATE_READ_ACK:
            size = pn532_dataread(pn532_cmd_buffer);
            mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
            mode_data.cardscan.pn532_cmd.write_size = size;
            mode_data.cardscan.pn532_cmd.read_size = 6;
            mode_data.cardscan.pn532_cmd.callback = &pn532_spi_callback_ack;

            spi_enqueue(&mode_data.cardscan.pn532_cmd);
            pn532_cmd_state = PN352_CMD_STATE_READ_ACK_IN_FLIGHT;    
            break;
            
        case PN352_CMD_STATE_READ_ACK_IN_FLIGHT:
            break;                        
            
        case PN532_CMD_STATE_WAIT_FOR_RESPONSE_IRQ:
            if (pn532_cmd_read_size == 0) {
                pn532_cmd_state = PN532_CMD_STATE_DONE;
                pn532_cmd_callback(true);
            } else {   
                if (!kpin_read(KPIN_C4)) {
                    pn532_cmd_state++;
                }
            }
            break;        
            
        case PN532_CMD_STATE_READ_RESPONSE:                
            size = pn532_dataread(pn532_cmd_buffer);
            mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
            mode_data.cardscan.pn532_cmd.write_size = 1;
            mode_data.cardscan.pn532_cmd.read_size = pn532_cmd_read_size;
            mode_data.cardscan.pn532_cmd.callback = &pn532_spi_callback_response;
            
            spi_enqueue(&mode_data.cardscan.pn532_cmd);
            pn532_cmd_state = PN532_CMD_STATE_READ_RESPONSE_IN_FLIGHT;
            break;
            
        case PN532_CMD_STATE_READ_RESPONSE_IN_FLIGHT:
            break;             
            
        case PN532_CMD_STATE_DONE:
            // No action
            break;
    }    
}

spi_command_t *pn532_spi_callback_write(spi_command_t *cmd) {
    pn532_cmd_state++;   
    
    return NULL;
}

spi_command_t *pn532_spi_callback_ack(spi_command_t *cmd) {
    if (cmd->buffer[0] == 0x00 && cmd->buffer[1] == 0x00 && cmd->buffer[2] == 0xff && cmd->buffer[3] == 0x00 && cmd->buffer[4] == 0xff && cmd->buffer[5] == 0x00) {
        pn532_cmd_state++;        
    } else {
        pn532_cmd_callback(false);
    }        
    
    return NULL;
}

spi_command_t *pn532_spi_callback_response(spi_command_t *cmd) {        
    pn532_cmd_state++;
    pn532_cmd_callback(true);
    
    return NULL;
}

#define PN532_SPI_DATAWRITE 0x01
#define PN532_PREAMBLE      0x00
#define PN532_STARTCODE1    0x00
#define PN532_STARTCODE2    0xff
#define PN532_POSTAMBLE     0x00
#define PN532_HOSTTOPN532   0xd4

uint8_t *pn532_packet_prepare(uint8_t *buff, uint8_t cmd_len) { 
    // Take into account PN532_HOSTTOPN532.
    cmd_len++;
    
    buff[0] = PN532_SPI_DATAWRITE;
    buff[1] = PN532_PREAMBLE;
    buff[2] = PN532_STARTCODE1;
    buff[3] = PN532_STARTCODE2;
    buff[4] = cmd_len;
    buff[5] = (~cmd_len) + 1;
    buff[6] = PN532_HOSTTOPN532;
       
    buff[6 + cmd_len] = 0x00;
    buff[7 + cmd_len] = PN532_POSTAMBLE;
    
    return &buff[7];
}

void pn532_packet_finalise(uint8_t *buff) {
     uint8_t checksum = PN532_HOSTTOPN532;        
     uint8_t cmd_len = buff[4];
     
     for (uint8_t i = 0; i < cmd_len; i++) {
         checksum += buff[7 + i];
     }
     
     buff[6 + cmd_len] = (~checksum) + 1;    
}

uint8_t pn532_size(uint8_t cmd_len) {
    return 9 + cmd_len;
}

#define PN532_COMMAND_SAMCONFIGURATION 0x14

uint8_t pn532_samconfigure(uint8_t *buff) {
    uint8_t *cmd = pn532_packet_prepare(buff, 4);
    cmd[0] = PN532_COMMAND_SAMCONFIGURATION;
    cmd[1] = 0x01; // Normal mode
    cmd[2] = 0x00; // No timeout, only used for Virtual Card.
    cmd[3] = 0x01; // Enable IRQ
    pn532_packet_finalise(buff);
    
    return pn532_size(4);
}

#define PN532_COMMAND_GETFIRMWAREVERSION 0x02

uint8_t pn532_getfirmwareversion(uint8_t *buff) {
    uint8_t *cmd = pn532_packet_prepare(buff, 1);
    cmd[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    pn532_packet_finalise(buff);
    
    return pn532_size(1);
}

#define PN532_COMMAND_INLISTPASSIVETARGET 0x4A

uint8_t pn532_inlistpassivetarget(uint8_t *buff) {
    uint8_t *cmd = pn532_packet_prepare(buff, 3);
    cmd[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    cmd[1] = 0x01; // Count
    cmd[2] = 0x00; // 106 kbps type A (ISO/IEC14443 Type A)
    pn532_packet_finalise(buff);
    
    return pn532_size(3);
}

#define PN532_SPI_STATREAD 0x02

uint8_t pn532_statread(uint8_t *buff) {
    buff[0] = PN532_SPI_STATREAD;   
    return 1;
}

#define PN532_SPI_DATAREAD 0x03

uint8_t pn532_dataread(uint8_t *buff) {
    buff[0] = PN532_SPI_DATAREAD;   
    return 1;
}

#define PN532_COMMAND_INDATAEXCHANGE 0x40

#define MIFARE_ULTRALIGHT_CMD_WRITE 0xa2

uint8_t pn532_mifareultralight_write(uint8_t *buff, uint8_t page, uint8_t *data) {    
    uint8_t *cmd = pn532_packet_prepare(buff, 8);
    cmd[0] = PN532_COMMAND_INDATAEXCHANGE;
    cmd[1] = 0x01;                          // Target card 1
    cmd[2] = MIFARE_ULTRALIGHT_CMD_WRITE;   // Write ultra light block.
    cmd[3] = page;                          // Page to write.
    cmd[4] = data[0];
    cmd[5] = data[1];
    cmd[6] = data[2];
    cmd[7] = data[3];   
    pn532_packet_finalise(buff);
    
    return pn532_size(8);
}

#define MIFARE_CMD_READ 0x30

uint8_t pn532_mifareultralight_read(uint8_t *buff, uint8_t page) {    
    uint8_t *cmd = pn532_packet_prepare(buff, 4);
    cmd[0] = PN532_COMMAND_INDATAEXCHANGE;
    cmd[1] = 0x01;                          // Target card 1
    cmd[2] = MIFARE_CMD_READ;               // Read ultra light block.
    cmd[3] = page;                          // Page to write.
    pn532_packet_finalise(buff);
    
    return pn532_size(4);
}
