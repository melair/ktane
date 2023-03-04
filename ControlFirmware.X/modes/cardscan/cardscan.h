#ifndef CARDSCAN_H
#define	CARDSCAN_H

#include "../../spi.h"

void cardscan_initialise(void);

#define CARDSCAN_SPI_BUFFER 32
#define CARDSCAN_POOL_SIZE  16

#define CARDSCAN_MFU_UID_LEN 7

typedef struct {
    struct {
        uint8_t state;
        uint32_t wait_time;
        
        struct {
            uint8_t         buffer[CARDSCAN_SPI_BUFFER];
            spi_command_t   cmd;
            uint8_t         device_id;
        } spi;
        
        struct {
            uint8_t         state;
            uint8_t         write_size;
            uint8_t         read_size;
            uint8_t         *buffer;
            void            (*callback)(bool);            
        } cmd;
    } pn532;
    
    struct {                
        uint32_t            scan_tick;
        uint8_t             scan_uid[CARDSCAN_MFU_UID_LEN];        
    } rfid;
    
    struct {
        uint8_t             last_id;
        bool                last_updated;

        bool                programming;
        uint8_t             programming_id;
        bool                programming_update;            
    } cards;    
} mode_cardscan_t;

#endif	/* CARDSCAN_H */

