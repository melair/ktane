#include <xc.h>
#include "pn532_packet.h"

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
