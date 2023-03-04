#ifndef CARDSCAN_H
#define	CARDSCAN_H

#include "../../spi.h"

void cardscan_initialise(void);

#define CARDSCAN_SPI_BUFFER 32
#define CARDSCAN_POOL_SIZE  16

#define CARDSCAN_MFU_UID_LEN 7

typedef struct {
    uint8_t spi_buffer[CARDSCAN_SPI_BUFFER];
    spi_command_t spi_cmd;
    uint8_t spi_device_id;
    
    uint8_t pn532_state;
    uint32_t pn532_wait_time;
       
    uint8_t pn532_cmd_state;
    uint8_t pn532_cmd_write_size;
    uint8_t pn532_cmd_read_size;
    uint8_t *pn532_cmd_buffer;
    void (*pn532_cmd_callback)(bool);
    
    uint32_t rfid_scan_tick;
    uint8_t rfid_scan_uid[CARDSCAN_MFU_UID_LEN];
    
    uint8_t card_id;
    bool card_new;
    
    bool programming;
    uint8_t programming_id;
} mode_cardscan_t;

#endif	/* CARDSCAN_H */

