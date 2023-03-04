#include <xc.h>
#include <stdbool.h>
#include "cardscan.h"
#include "pn532.h"
#include "pn532_cmd.h"
#include "pn532_packet.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../spi.h"
#include "../../tick.h"
#include "../../peripherals/lcd.h"

#define CARDSCAN_RNG_MASK 0x96ea865c

void cardscan_service(void);
void cardscan_service_setup(bool first);

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
    
    /* Load the big font into the LCD. */
    lcd_load_big();
    
    /* Initialise SPI, register RFID. */
    kpin_mode(KPIN_C0, PIN_OUTPUT, false);
    kpin_mode(KPIN_C1, PIN_INPUT, false);
    kpin_mode(KPIN_C2, PIN_OUTPUT, false);
    kpin_mode(KPIN_C3, PIN_OUTPUT, false);    
        
    /* Set CS to high. */
    kpin_write(KPIN_C3, true);
    
    /* Initialise SPI. */
    mode_data.cardscan.pn532.spi.device_id = spi_register(&cardscan_device);
    
    /* Initialise IRQ. */
    kpin_mode(KPIN_C4, PIN_INPUT, false);
    
    /* Register callbacks. */
    mode_register_callback(GAME_ALWAYS, cardscan_service, NULL);
    mode_register_callback(GAME_SETUP, cardscan_service_setup, &tick_20hz);
}

void cardscan_service(void) {
     lcd_service();   
     pn532_cmd_service();
     pn532_service();
     
     if (mode_data.cardscan.cards.programming && (tick_2hz || mode_data.cardscan.cards.programming_update)) {
         lcd_update(0, 2, 12, "Program Card");
         lcd_update(1, 4, 8, "XX of XX");
         
         lcd_number(1, 4, 2, mode_data.cardscan.cards.programming_id + 1);
         lcd_number(1, 10, 2, CARDSCAN_POOL_SIZE);
         
         lcd_sync();
         
         mode_data.cardscan.cards.programming_update = false;
     }
}

void cardscan_service_setup(bool first) {
    if (!this_module->ready && !mode_data.cardscan.cards.programming) {
        game_module_ready(true);
    }
}