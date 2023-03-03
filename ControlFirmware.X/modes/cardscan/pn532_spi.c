#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pn532_spi.h"
#include "pn532_packet.h"
#include "../../mode.h"
#include "../../spi.h"

spi_command_t *pn532_spi_callback_ack(spi_command_t *cmd);
spi_command_t *pn532_spi_callback_response(spi_command_t *cmd);
spi_command_t *pn532_spi_callback_write(spi_command_t *cmd);

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

