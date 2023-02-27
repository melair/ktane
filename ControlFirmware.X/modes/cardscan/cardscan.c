#include <xc.h>
#include "cardscan.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../spi.h"
#include "../../tick.h"
#include "../../peripherals/lcd.h"

#define CARDSCAN_RNG_MASK 0x96ea865c

void cardscan_service(void);
void pn532_service(void);
void pn532_service_configured_callback(void);

void pn532_spi_service(void);
void pn532_spi_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(void));
spi_command_t *pn532_spi_verify_ack(spi_command_t *cmd);
spi_command_t *pn532_spi_response(spi_command_t *cmd);

uint8_t *pn532_packet_prepare(uint8_t *buff, uint8_t cmd_len);
void pn532_packet_finalise(uint8_t *buff);
uint8_t pn532_size(uint8_t cmd_len);

uint8_t pn532_samconfigure(uint8_t *buff);
uint8_t pn532_inlistpassivetarget(uint8_t *buff);
uint8_t pn532_statread(uint8_t *buff);
uint8_t pn532_dataread(uint8_t *buff);

const spi_device_t cardscan_device = {
    .clk_pin = KPIN_C0,
    .miso_pin = KPIN_C1,
    .mosi_pin = KPIN_C2,
    .cs_pin = KPIN_C3,
    .baud = SPI_BAUD_800_KHZ 
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
    kpin_mode(KPIN_C4, PIN_INPUT, true);
    
    /* Register callbacks. */
    mode_register_callback(GAME_ALWAYS, cardscan_service, NULL);

}

void cardscan_service(void) {
     lcd_service();   
     pn532_spi_service();
     pn532_service();
}

#define PN532_STATE_UNINITIALISED 0
#define PN532_STATE_WAKING_UP     1
#define PN532_STATE_CONFIGURING   2
#define PN532_STATE_CONFIGURED    3
#define PN532_STATE_CARD_DETECT   4

#define PN532_CMD_STATE_WRITE            0
#define PN532_CMD_STATE_WAITING_ACK      1
#define PN532_CMD_STATE_READ_ACK         2
#define PN532_CMD_STATE_WAITING_RESPONSE 3
#define PN532_CMD_STATE_READ_RESPONSE    4
#define PN532_CMD_STATE_DONE             5

uint8_t pn532_cmd_state;
uint8_t pn532_cmd_write_size;
uint8_t pn532_cmd_read_size;
uint8_t *pn532_cmd_buffer;
void (*pn532_cmd_callback)(void);

void pn532_service(void) {
    switch(mode_data.cardscan.pn532_state) {
        case PN532_STATE_UNINITIALISED:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_wakeup_time = tick_value;
            mode_data.cardscan.pn532_state = PN532_STATE_WAKING_UP;
            break;
        case PN532_STATE_WAKING_UP:
            if (mode_data.cardscan.pn532_wakeup_time + 10 < tick_value) {
                kpin_write(KPIN_C3, true);                
                mode_data.cardscan.pn532_state = PN532_STATE_CONFIGURING;

                uint8_t size = pn532_samconfigure(&mode_data.cardscan.spi_buffer);
                pn532_spi_send(&mode_data.cardscan.spi_buffer, size, 9, &pn532_service_configured_callback);
            }
            break;
        case PN532_STATE_CONFIGURING:
            // No Action
            break;
        case PN532_STATE_CONFIGURED:
            // TODO: Start card detect loop.
            break;
           
    }        
}

void pn532_service_configured_callback(void) {
    if (mode_data.cardscan.spi_buffer[6] == 0x15) {
        mode_data.cardscan.pn532_state = PN532_STATE_CONFIGURED;
    } else {
        // Error condition.
    }       
}

void pn532_spi_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(void)) {
    pn532_cmd_state = PN532_CMD_STATE_WRITE;
    pn532_cmd_buffer = buff;
    pn532_cmd_write_size = write_size;
    pn532_cmd_read_size = read_size;
    pn532_cmd_callback = callback;
    
    pn532_spi_service();
}

bool pn532_irq_track = true;

void pn532_spi_service(void) {
    bool new_irq = false;
    bool current_reading = kpin_read(KPIN_C4);
    
    if (current_reading != pn532_irq_track) {
        if (!current_reading) {
            new_irq = true;
        }
        
        pn532_irq_track = current_reading;
    }
    
    switch(pn532_cmd_state) {
        case PN532_CMD_STATE_WRITE:
            mode_data.cardscan.pn532_cmd.device = mode_data.cardscan.spi_device;
            mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE;
            mode_data.cardscan.pn532_cmd.buffer = pn532_cmd_buffer;
            mode_data.cardscan.pn532_cmd.write_size = pn532_cmd_write_size;
            mode_data.cardscan.pn532_cmd.read_size = 0;
            mode_data.cardscan.pn532_cmd.callback = NULL;
            
            spi_enqueue(&mode_data.cardscan.pn532_cmd);
            pn532_cmd_state++;
            break;
            
        case PN532_CMD_STATE_WAITING_ACK:
            if (new_irq) {
                pn532_cmd_state++;
                
                uint8_t size = pn532_dataread(pn532_cmd_buffer);
                mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
                mode_data.cardscan.pn532_cmd.write_size = 1;
                mode_data.cardscan.pn532_cmd.read_size = 6;
                mode_data.cardscan.pn532_cmd.callback = &pn532_spi_verify_ack;
            }
            
            break;
            
        case PN532_CMD_STATE_READ_ACK:            
            // No action
            break;            
            
        case PN532_CMD_STATE_WAITING_RESPONSE:
            if (new_irq) {
                pn532_cmd_state++;
                
                uint8_t size = pn532_dataread(pn532_cmd_buffer);
                mode_data.cardscan.pn532_cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
                mode_data.cardscan.pn532_cmd.write_size = 1;
                mode_data.cardscan.pn532_cmd.read_size = pn532_cmd_read_size;
                mode_data.cardscan.pn532_cmd.callback = &pn532_spi_response;
            }            
            break;
            
        case PN532_CMD_STATE_READ_RESPONSE:
            // No action            
            break;
            
        case PN532_CMD_STATE_DONE:
            // No action
            break;
    }    
}

spi_command_t *pn532_spi_verify_ack(spi_command_t *cmd) {
    if (cmd->buffer[0] == 0x00 && cmd->buffer[1] == 0x00 && cmd->buffer[2] == 0xff && cmd->buffer[3] == 0x00 && cmd->buffer[4] == 0xff && cmd->buffer[5] == 0x00) {
        pn532_cmd_state++;        
    } else {
        // error condition
    }        
    
    return NULL;
}

spi_command_t *pn532_spi_response(spi_command_t *cmd) {        
    pn532_cmd_state++;
    pn532_cmd_callback();
    
    return NULL;
}

#define PN532_SPI_DATAWRITE 0x01
#define PN532_PREAMBLE      0x00
#define PN532_STARTCODE1    0x00
#define PN532_STARTCODE2    0xff
#define PN532_POSTAMBLE     0x00
#define PN532_HOSTTOPN532   0xd4

uint8_t *pn532_packet_prepare(uint8_t *buff, uint8_t cmd_len) {   
    buff[0] = PN532_SPI_DATAWRITE;
    buff[1] = PN532_PREAMBLE;
    buff[2] = PN532_STARTCODE1;
    buff[3] = PN532_STARTCODE2;
    buff[4] = cmd_len;
    buff[5] = (~cmd_len) + 1;
    buff[6] = PN532_HOSTTOPN532;
       
    buff[7 + cmd_len] = 0x00;
    buff[8 + cmd_len] = PN532_POSTAMBLE;
    
    return &buff[7];
}

void pn532_packet_finalise(uint8_t *buff) {
     uint8_t checksum = PN532_PREAMBLE + PN532_STARTCODE1 + PN532_STARTCODE2 + PN532_HOSTTOPN532;        
     uint8_t cmd_len = buff[4];
     
     for (uint8_t i = 0; i < cmd_len; i++) {
         checksum += buff[7 + i];
     }
     
     buff[7 + cmd_len] = ~checksum;    
}

uint8_t pn532_size(uint8_t cmd_len) {
    return 9 + cmd_len;
}

#define PN532_COMMAND_SAMCONFIGURATION 0x14

uint8_t pn532_samconfigure(uint8_t *buff) {
    uint8_t *cmd = pn532_packet_prepare(buff, 4);
    cmd[0] = PN532_COMMAND_SAMCONFIGURATION;
    cmd[1] = 0x01; // Normal mode
    cmd[2] = 0x14; // Timeout 50ms * 20 = 1 second
    cmd[3] = 0x01; // Enable IRQ
    pn532_packet_finalise(buff);
    
    return pn532_size(4);
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

#define PN532_SPI_DATAREAD 0x03

uint8_t pn532_dataread(uint8_t *buff) {
    buff[0] = PN532_SPI_DATAREAD;   
    return 1;
}