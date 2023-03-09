#include <xc.h>
#include <stdbool.h>
#include "cardscan.h"
#include "pn532.h"
#include "pn532_cmd.h"
#include "pn532_packet.h"
#include "../../buzzer.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../spi.h"
#include "../../tick.h"
#include "../../peripherals/lcd.h"

#define CARDSCAN_RNG_MASK 0x96ea865c

void cardscan_service(bool first);
void cardscan_service_idle(bool first);
void cardscan_service_setup(bool first);
void cardscan_service_running(bool first);
void cardscan_special_function(uint8_t special_fn);

#define CARDSCAN_CHARACTERS 4

const uint8_t cardscan_character[CARDSCAN_CHARACTERS][LCD_CHARACTER_ROWS] = {
    { // Heart
        0b00000,
        0b01010,
        0b11111,
        0b11111,
        0b11111,
        0b01110,
        0b00100,
        0b00000
    },
    { // Empty
        0b00000,
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b01110,
        0b00000,
        0b00000
    },
    { // Filled
        0b00000,
        0b01110,
        0b11111,
        0b11111,
        0b11111,
        0b01110,
        0b00000,
        0b00000
    },
    { // Cross
        0b00000,
        0b10001,
        0b01010,
        0b00100,
        0b01010,
        0b10001,
        0b00000,
        0b00000
    }
};

#define CARDSCAN_MAX_NAME       20

const uint8_t cardscan_names[CARDSCAN_CARD_COUNT][CARDSCAN_MAX_NAME] = {
    "Acheron",
    "Kontrollbeamter",
    "Nyx",
    "Obaida",
    "Lloyd Davis",
    "Judge Harris",
    "Nine",
    "Wuming",
    "Richmond Avenal",
    "Josephus Miller",
    "Breeze",
    "La Succubus",
    "Shaykh Alwaqt",
    "Lady of the Night",
    "Colonel Riker",
    "Reaver",
    "Nozomi",
    "Mena",
    "Duck",
    "Packet",
    "Jamie the Needle",
    "The Librarian",
    "Blackbird",
    "El Padre del Diablo"
};

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
    mode_register_callback(GAME_IDLE, cardscan_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, cardscan_service_setup, &tick_20hz);
    mode_register_callback(GAME_RUNNING, cardscan_service_running, &tick_20hz);

    mode_register_special_fn_callback(&cardscan_special_function);
    
    /* Clear LCD. */
    lcd_clear();
    lcd_sync();
    
    /* Load custom symbols into RAM. */
    for (uint8_t i = 0; i < CARDSCAN_CHARACTERS; i++) {
        lcd_custom_character(i, &cardscan_character[i]);
    }
}

void cardscan_service(bool first) {
     lcd_service();   
     pn532_cmd_service();
     pn532_service();
}

void cardscan_service_idle(bool first) {
    if (mode_data.cardscan.cards.programming && (tick_2hz || mode_data.cardscan.cards.programming_update)) {
        lcd_clear();

        lcd_update(0, 2, 12, "Program Card");
        lcd_update(1, 4, 8, "XX of XX");

        lcd_number(1, 4, 2, mode_data.cardscan.cards.programming_id + 1);
        lcd_number(1, 10, 2, CARDSCAN_CARD_COUNT);

        lcd_sync();

        if (mode_data.cardscan.cards.programming_update) {
            mode_data.cardscan.cards.programming_update = false;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 100);
        }
    }
}

#define CARDSCAN_INITIAL_LIVES 3

void cardscan_service_setup(bool first) {
    if (first) {
        lcd_clear();
        lcd_sync();
        
        mode_data.cardscan.cards.programming = false;
        
        mode_data.cardscan.cards.wanted_id = ((uint8_t) (game.module_seed & 0xff)) % CARDSCAN_CARD_COUNT;
        mode_data.cardscan.cards.lives = CARDSCAN_INITIAL_LIVES;
        mode_data.cardscan.cards.last_id = 0xff;  
    }
    
    if (!this_module->ready && !mode_data.cardscan.cards.programming) {
        game_module_ready(true);            
    }
}

void cardscan_service_running(bool first) {    
    bool redraw = false;
    
    if (first) {
        mode_data.cardscan.cards.scanned_updated = false;        
        redraw = true;
    }
    
    if (mode_data.cardscan.cards.scanned_updated) {
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 100);
                        
        mode_data.cardscan.cards.scanned_updated = false;
        redraw = true;
        
        if (mode_data.cardscan.cards.scanned_id != mode_data.cardscan.cards.wanted_id) {            
            if (mode_data.cardscan.cards.lives == 0) {
                game_module_strike(1);
            } else {
                mode_data.cardscan.cards.lives--;
            }
        } else {
            game_module_solved(true);
        }
        
        mode_data.cardscan.cards.last_id = mode_data.cardscan.cards.scanned_id;
    }
        
    if (redraw || tick_2hz) {
        if (tick_2hz) {
            mode_data.cardscan.cards.flash = !mode_data.cardscan.cards.flash;
        }
        
        lcd_clear();
        
        if (mode_data.cardscan.cards.last_id != 0xff) {
            uint8_t size = 0;
            
            for (size = 0; size < 16; size++) {
                if (cardscan_names[mode_data.cardscan.cards.last_id][size] == '\0') {
                    break;
                }
            }
            
            uint8_t offset = 8 - (size / 2);            
            lcd_update(0, offset, size, &cardscan_names[mode_data.cardscan.cards.last_id]);            
            
            bool fact_a = (mode_data.cardscan.cards.wanted_id / 6) == (mode_data.cardscan.cards.last_id / 6);
            bool fact_b = ((mode_data.cardscan.cards.wanted_id / 2) % 3) == ((mode_data.cardscan.cards.last_id / 2) % 3);
            bool fact_c = (mode_data.cardscan.cards.wanted_id % 2) == (mode_data.cardscan.cards.last_id % 2);
            
            lcd_update(1, 13, 1, (fact_a ? "\2": "\1"));
            lcd_update(1, 14, 1, (fact_b ? "\2": "\1"));
            lcd_update(1, 15, 1, (fact_c ? "\2": "\1"));
        } else {
            lcd_update(0, 2, 12, "Scan ID Card");
        }        
        
        for (uint8_t i = 0; i < CARDSCAN_INITIAL_LIVES; i++) {
            bool heart = (i <  mode_data.cardscan.cards.lives);
            
            if (heart || mode_data.cardscan.cards.flash) {
                lcd_update(1, i, 1, (heart ? "\0" : "\3"));
            }
        }
        
        lcd_sync();
    }    
}

void cardscan_special_function(uint8_t special_fn) {
    switch(special_fn) {
        case 0:
            mode_data.cardscan.cards.programming = !mode_data.cardscan.cards.programming;
            
            if (!mode_data.cardscan.cards.programming) {
                /* Clear LCD. */
                lcd_clear();
                lcd_sync();
            } else {               
                /* Set initial card back to 0. */
                mode_data.cardscan.cards.programming_id = 0;
            }
            break;
    }
}