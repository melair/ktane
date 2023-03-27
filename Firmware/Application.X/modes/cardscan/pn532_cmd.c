#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pn532_cmd.h"
#include "pn532_packet.h"
#include "../../mode.h"
#include "../../hal/spi.h"

spi_command_t *pn532_cmd_callback_ack(spi_command_t *cmd);
spi_command_t *pn532_cmd_callback_response(spi_command_t *cmd);
spi_command_t *pn532_cmd_callback_write(spi_command_t *cmd);

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

const spi_device_t cardscan_device = {
    .clk_pin = KPIN_C0,
    .miso_pin = KPIN_C1,
    .mosi_pin = KPIN_C2,
    .cs_pin = KPIN_C3,
    .baud = SPI_BAUD_800_KHZ,
    .lsb_first = 1,
    .cke = 1,
};

void pn532_cmd_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(bool)) {
    if (write_size > 0) {
        mode_data.cardscan.pn532.cmd.state = PN532_CMD_STATE_WRITE;
    } else {
        mode_data.cardscan.pn532.cmd.state = PN532_CMD_STATE_READ_RESPONSE;
    }
    mode_data.cardscan.pn532.cmd.buffer = buff;
    mode_data.cardscan.pn532.cmd.write_size = write_size;
    mode_data.cardscan.pn532.cmd.read_size = read_size;
    mode_data.cardscan.pn532.cmd.callback = callback;
    
    pn532_cmd_service();
}

void pn532_cmd_service(void) {   
    uint8_t size = 0;
    
    switch(mode_data.cardscan.pn532.cmd.state) {
        case PN532_CMD_STATE_IDLE:
            // No Action
            break;
            
        case PN532_CMD_STATE_WRITE:           
            mode_data.cardscan.pn532.spi.cmd.device = &cardscan_device;
            mode_data.cardscan.pn532.spi.cmd.operation = SPI_OPERATION_WRITE;
            mode_data.cardscan.pn532.spi.cmd.buffer = mode_data.cardscan.pn532.cmd.buffer;
            mode_data.cardscan.pn532.spi.cmd.write_size = mode_data.cardscan.pn532.cmd.write_size;
            mode_data.cardscan.pn532.spi.cmd.read_size = 0;
            mode_data.cardscan.pn532.spi.cmd.callback = &pn532_cmd_callback_write;
            
            spi_enqueue(&mode_data.cardscan.pn532.spi.cmd);
            mode_data.cardscan.pn532.cmd.state = PN532_CMD_STATE_WRITE_IN_FLIGHT;
            break;
            
        case PN532_CMD_STATE_WRITE_IN_FLIGHT:
            break;
            
        case PN532_CMD_STATE_WAIT_FOR_ACK_IRQ:
            if (!kpin_read(KPIN_C4)) {
                mode_data.cardscan.pn532.cmd.state = PN352_CMD_STATE_READ_ACK;
            }
            break;
                       
        case PN352_CMD_STATE_READ_ACK:
            size = pn532_dataread(mode_data.cardscan.pn532.cmd.buffer);
            mode_data.cardscan.pn532.spi.cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
            mode_data.cardscan.pn532.spi.cmd.write_size = size;
            mode_data.cardscan.pn532.spi.cmd.read_size = 6;
            mode_data.cardscan.pn532.spi.cmd.callback = &pn532_cmd_callback_ack;

            spi_enqueue(&mode_data.cardscan.pn532.spi.cmd);
            mode_data.cardscan.pn532.cmd.state = PN352_CMD_STATE_READ_ACK_IN_FLIGHT;    
            break;
            
        case PN352_CMD_STATE_READ_ACK_IN_FLIGHT:
            break;                        
            
        case PN532_CMD_STATE_WAIT_FOR_RESPONSE_IRQ:
            if (mode_data.cardscan.pn532.cmd.read_size == 0) {
                mode_data.cardscan.pn532.cmd.state = PN532_CMD_STATE_DONE;
                mode_data.cardscan.pn532.cmd.callback(true);
            } else {   
                if (!kpin_read(KPIN_C4)) {
                    mode_data.cardscan.pn532.cmd.state++;
                }
            }
            break;        
            
        case PN532_CMD_STATE_READ_RESPONSE:                
            size = pn532_dataread(mode_data.cardscan.pn532.cmd.buffer);
            mode_data.cardscan.pn532.spi.cmd.operation = SPI_OPERATION_WRITE_THEN_READ;
            mode_data.cardscan.pn532.spi.cmd.write_size = 1;
            mode_data.cardscan.pn532.spi.cmd.read_size = mode_data.cardscan.pn532.cmd.read_size;
            mode_data.cardscan.pn532.spi.cmd.callback = &pn532_cmd_callback_response;
            
            spi_enqueue(&mode_data.cardscan.pn532.spi.cmd);
            mode_data.cardscan.pn532.cmd.state = PN532_CMD_STATE_READ_RESPONSE_IN_FLIGHT;
            break;
            
        case PN532_CMD_STATE_READ_RESPONSE_IN_FLIGHT:
            break;             
            
        case PN532_CMD_STATE_DONE:
            // No action
            break;
    }    
}

spi_command_t *pn532_cmd_callback_write(spi_command_t *cmd) {
    mode_data.cardscan.pn532.cmd.state++;   
    
    return NULL;
}

spi_command_t *pn532_cmd_callback_ack(spi_command_t *cmd) {
    if (cmd->buffer[0] == 0x00 && cmd->buffer[1] == 0x00 && cmd->buffer[2] == 0xff && cmd->buffer[3] == 0x00 && cmd->buffer[4] == 0xff && cmd->buffer[5] == 0x00) {
        mode_data.cardscan.pn532.cmd.state++;        
    } else {
        mode_data.cardscan.pn532.cmd.callback(false);
    }        
    
    return NULL;
}

spi_command_t *pn532_cmd_callback_response(spi_command_t *cmd) {        
    mode_data.cardscan.pn532.cmd.state++;
    mode_data.cardscan.pn532.cmd.callback(true);
    
    return NULL;
}

