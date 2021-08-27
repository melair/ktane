#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "edgework.h"
#include "rng.h"
#include "lcd.h"

#define EDGEWORK_RNG_MASK 0x1e4ab852

#define MAX_EDGEWORK        7
#define DEFAULT_EDGEWORK    5
#define SERIAL_LENGTH       6

#define SLOT_PREFIX_MASK        0b11100000
#define SLOT_PREFIX_NOTHING     0b00000000
#define SLOT_PREFIX_PORT        0b00100000
#define SLOT_PREFIX_INDICATOR   0b01000000
#define SLOT_PREFIX_BATTERY     0b01100000
#define SLOT_PREFIX_SERIAL      0b11100000

uint8_t edgework_slots[MAX_EDGEWORK];
uint8_t edgework_serial[SERIAL_LENGTH];
bool edgework_serial_has_vowel = false;
uint8_t edgework_battery = 0;

const uint8_t indicator_name[INDICATOR_MAX + 1][3] = {
  "SND",
  "CLR",
  "CAR",
  "IND",
  "FRQ",
  "SIG",
  "NSA",
  "MSA",
  "TRN",
  "BOB",
  "FRK",
};

const uint8_t port_name[PORT_MAX + 1][4] = {
  "DVI ",
  "PARA",
  "PS2",
  "RJ45",
  "SERI",
  "RCA",
};

/**
 * Generate new edgework for the game.
 * 
 * @param seed seed to generate edgework for
 * @param difficulty level to set edgework for
 */
void edgework_generate(uint32_t seed, uint8_t difficulty) {       
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        edgework_slots[i] = 0;
    }
    
    edgework_battery = 0;
    
    /* Generate serial number. */
    for (uint8_t i = 0; i < (SERIAL_LENGTH - 1); i++) {
        /* 35, not 36, Y is never present. */
        uint8_t t = rng_generate8(&seed, EDGEWORK_RNG_MASK) % 35;
        
        /* If it would be Y, bump to Z.*/
        if (t == 34) {
            t++;
        }
        
        if (t < 10) {
            edgework_serial[i] = t + '0';
        } else {
            edgework_serial[i] = t - 10 + 'A';
        }
        
        switch (edgework_serial[i]) {
            case 'A':
            case 'E':
            case 'I':
            case 'O':
            case 'U':
                edgework_serial_has_vowel = true;
                break;
        }
    }
    
    edgework_serial[SERIAL_LENGTH - 1] = (rng_generate8(&seed, EDGEWORK_RNG_MASK) % 10) + '0';
    
    edgework_slots[0] = SLOT_PREFIX_SERIAL;
       
    /* Generate edgework. */
    for (uint8_t i = 1; i < DEFAULT_EDGEWORK; i++ ){
        uint8_t type = rng_generate8(&seed, EDGEWORK_RNG_MASK);
        uint8_t t = rng_generate8(&seed, EDGEWORK_RNG_MASK);
        
        switch(type % 3) {
            case 0:
                t = t % (PORT_MAX + 1);                
                edgework_slots[i] = SLOT_PREFIX_PORT | t;
                break;
            case 1:              
                t = t % (INDICATOR_MAX + 1);                
                edgework_slots[i] = SLOT_PREFIX_INDICATOR | t;
                
                /* 60% chance the indicator is lit. */
                if ((rng_generate8(&seed, EDGEWORK_RNG_MASK) % 5) > 1) {
                    edgework_slots[i] |= 0b00010000;
                }
                
                break;
            case 2:              
                t = t % 2;
                edgework_slots[i] = SLOT_PREFIX_BATTERY | t;
                edgework_battery += (t + 1);
                break;
        }
    }    
    
    /* Shuffle edgework. */
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        uint8_t s = rng_generate8(&seed, EDGEWORK_RNG_MASK) % MAX_EDGEWORK;
        uint8_t t = edgework_slots[i];        
        edgework_slots[i] = edgework_slots[s];
        edgework_slots[s] = t;
    }
}

/**
 * Update devices to display game edge work.
 */
void edgework_display(void) {
    lcd_clear();
    
    const uint8_t *serial = "Serial Number:";    
    lcd_update(0, 0, 14, serial);
    lcd_update(1, 0, SERIAL_LENGTH, &edgework_serial);
                    
//    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
//        uint8_t r = i / 2;
//        uint8_t c = (i % 2) * 10;
//        
//        switch(edgework_slots[i] & SLOT_PREFIX_MASK) {
//            case SLOT_PREFIX_PORT:
//                t[0] = 'P';
//                lcd_update(r, c, 1, &t);                
//                
//                uint8_t port = edgework_slots[i] & 0b00001111;
//                lcd_update(r, c+2, 4, &port_name[port]); 
//                break;
//            case SLOT_PREFIX_INDICATOR:
//                t[0] = 'I';
//                
//                if ((edgework_slots[i] & 0b00010000) == 0b00010000) {
//                    t[5] = '+';
//                }
//                
//                lcd_update(r, c, 6, &t);
//                
//                uint8_t ind = edgework_slots[i] & 0b00001111;
//                lcd_update(r, c+2, 3, &indicator_name[ind]);               
//                
//                break;
//            case SLOT_PREFIX_BATTERY:
//                t[0] = 'B';
//                t[1] = ' ';
//                t[2] = (edgework_slots[i] & ~(SLOT_PREFIX_MASK)) + 1 + '0';
//                lcd_update(r, c, 3, &t);
//                break;
//            case SLOT_PREFIX_SERIAL:
//                lcd_update(r, c, SERIAL_LENGTH, &edgework_serial);
//                break;                        
//        }
//    }
//    
    lcd_sync();
}

/**
 * Check to see if the indicator specified has the lit state queried.
 * 
 * @param ind indicator to check for
 * @param lit true if indicator must be lit
 * @return true if the indicator is present and has the state required
 */
bool edgework_indicator_present(indicator_t ind, bool lit) {
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        if ((edgework_slots[i] & SLOT_PREFIX_INDICATOR) != SLOT_PREFIX_INDICATOR) {
            continue;
        }
        
        uint8_t indE = edgework_slots[i] & 0b00001111;
        bool litE = ((edgework_slots[i] & 0b000100000) == 0b000100000);        
        
        if (indE == ind && litE == lit) {
            return true;
        }
    }        
        
    return false;
}

/**
 * Get the battery count.
 * 
 * @return total battery count
 */
uint8_t edgework_battery_count(void) {
    return edgework_battery;
}

/**
 * Check if serial has a vowel.
 * 
 * @return  true if serial has a vowel
 */
bool edgework_serial_vowel(void) {
    return edgework_serial_has_vowel;
}

/**
 * Get the last digit from serial number.
 * 
 * @return last digit of serial number, as number not ASCII
 */
uint8_t edgework_serial_last_digit(void) {
    return edgework_serial[SERIAL_LENGTH - 1] - '0';
}

/**
 * Check to see if the port specified is present.
 * 
 * @param port port to look for
 * @return true if port is present
 */
bool edgework_port_present(port_t port) {
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        if ((edgework_slots[i] & SLOT_PREFIX_PORT) != SLOT_PREFIX_PORT) {
            continue;
        }
        
        if ((edgework_slots[i] & ~(SLOT_PREFIX_PORT)) == port) {
            return true;
        }
    }
    
    return false;
}