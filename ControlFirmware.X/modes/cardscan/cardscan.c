#include <xc.h>
#include <stdbool.h>
#include "cardscan.h"
#include "pn532.h"
#include "pn532_spi.h"
#include "pn532_packet.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../spi.h"
#include "../../tick.h"
#include "../../peripherals/lcd.h"

#define CARDSCAN_RNG_MASK 0x96ea865c

void cardscan_service(void);

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
