#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "wires.h"
#include "../../argb.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../peripherals/ports.h"
#include "../../edgework.h"

#define WIRES_RNG_MASK 0xbde83a1f

typedef struct {
    uint16_t adc;

    struct {
        unsigned plug : 4;
        unsigned wire : 4;
    } colours;
} wirelookup_t;

#define WIRE_COLOUR_NONE    0
#define WIRE_COLOUR_RED     1
#define WIRE_COLOUR_YELLOW  2
#define WIRE_COLOUR_BLUE    3
#define WIRE_COLOUR_WHITE   4
#define WIRE_COLOUR_BLACK   5

#define WIRE_POSSIBLE_COUNT 12

#define NO_WIRE 255

/* Lookup table, ADC value to plug and wire colour. */
wirelookup_t wirelookup[WIRE_POSSIBLE_COUNT] = {
    { 0, WIRE_COLOUR_BLUE, WIRE_COLOUR_BLUE,},
    { 468, WIRE_COLOUR_BLUE, WIRE_COLOUR_RED,},
    { 821, WIRE_COLOUR_BLUE, WIRE_COLOUR_WHITE,},
    { 1163, WIRE_COLOUR_RED, WIRE_COLOUR_BLUE,},
    { 1484, WIRE_COLOUR_RED, WIRE_COLOUR_WHITE,},
    { 1853, WIRE_COLOUR_RED, WIRE_COLOUR_RED,},
    { 2141, WIRE_COLOUR_WHITE, WIRE_COLOUR_BLUE,},
    { 2434, WIRE_COLOUR_WHITE, WIRE_COLOUR_RED,},
    { 2811, WIRE_COLOUR_WHITE, WIRE_COLOUR_WHITE,},
    { 3125, WIRE_COLOUR_YELLOW, WIRE_COLOUR_YELLOW,},
    { 3416, WIRE_COLOUR_BLACK, WIRE_COLOUR_BLACK,},
    { 3752, WIRE_COLOUR_NONE, WIRE_COLOUR_NONE,},
};

const uint8_t wire_colors[6][3] = {
    { 0x00, 0x00, 0x00,},
    { 0xff, 0x00, 0x00,},
    { 0xff, 0x5f, 0x00,},
    { 0x00, 0x00, 0xff,},
    { 0xff, 0xff, 0xff,},
    { 0x00, 0xff, 0x00,},
};

/* Local function prototypes. */
void wires_service_idle(bool first);
void wires_service_setup(bool first);
void wires_service_start(bool first);
void wires_service_running(bool first);
void wires_service_adc(void);
void wires_calculate_solution_simple(void);
void wires_calculate_solution_complex(void);
void wires_set_cut(int8_t wire);
void wires_set_cut_color(uint8_t color, int8_t wire);
uint8_t wires_get_wire_count(void);
uint8_t wires_get_color_count(uint8_t color);
uint8_t wires_get_last_wire_color(void);

/**
 * Initialise Wires.
 */
void wires_initialise(void) {
    /* Register callbacks for mode. */
    mode_register_callback(GAME_IDLE, wires_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, wires_service_setup, &tick_20hz);
    mode_register_callback(GAME_START, wires_service_start, &tick_20hz);
    mode_register_callback(GAME_RUNNING, wires_service_running, &tick_100hz);

    /* Set wire port pins to input and ADC. */
    KTRISB |= 0b00111111;
    KANSELB |= 0b00111111;

    /* Configure ADC. */
    /* Right justify to read all 12-bits. */
    ADCON0bits.FM = 1;
    /* Use ADCRC clock, allows Sleep(), 1-6uS TAD. */
    ADCON0bits.CS = 1;
    /* Set acquisition change time, approximately 800 samples per second (100 per channel), assuming 3uS TAD, though range is 1-6uS. Though internal clock will limit at 100Hz. */
    ADACQ = 50;
    /* Enable ADC. */
    ADCON0bits.ON = 1;
}

/**
 * Service the module when idle.
 * 
 * @param first true if first timed called
 */
void wires_service_idle(bool first) {
}

/**
 * Service the module during the setup phase.
 * 
 * @param first true if first timed called
 */
void wires_service_setup(bool first) {
    if (first) {
        mode_data.wires.process.init_stage = 0;
        mode_data.wires.process.adc_check = 0;
    }

    wires_service_adc();

    if (mode_data.wires.process.init_stage == 1) {
        uint32_t seed = game.module_seed;

        mode_data.wires.complex = false; //(rng_generate8(&seed, WIRES_RNG_MASK % 2) == 0);

        /* Generate the required number of wires for a game, 3-6 wires. */
        uint8_t required_wires = 3 + (rng_generate8(&seed, WIRES_RNG_MASK) % 4);

        /* Check how many wires there are. */
        uint8_t current_wires = 0;

        /* Mark all wires required initially. */
        for (uint8_t i = 0; i < WIRES_COUNT; i++) {
            if (mode_data.wires.wires[i].wiretype != NO_WIRE) { 
               current_wires++;
                mode_data.wires.wires[i].required = true;
                mode_data.wires.wires[i].updated = 1;
                mode_data.wires.wires[i].cutneeded = 0;
            }            
        }

        if (current_wires > required_wires) {
            /* If there are too many wires, randomly chose some to remove. */
            uint8_t diff = current_wires - required_wires;

            while (diff > 0) {
                uint8_t i = rng_generate8(&seed, WIRES_RNG_MASK) % WIRES_COUNT;

                if (mode_data.wires.wires[i].wiretype != NO_WIRE) {
                    mode_data.wires.wires[i].required = false;
                    mode_data.wires.wires[i].updated = 1;
                    diff--;
                }
            }
        } else if (current_wires < required_wires) {
            /* If there are too few wires, randomly chose some to add. */
            uint8_t diff = required_wires - current_wires;

            while (diff > 0) {
                uint8_t i = rng_generate8(&seed, WIRES_RNG_MASK) % WIRES_COUNT;

                if (mode_data.wires.wires[i].wiretype == NO_WIRE) {
                    mode_data.wires.wires[i].required = true;
                    mode_data.wires.wires[i].updated = 1;
                    diff--;
                }
            }
        }
        
        if (mode_data.wires.complex) {
            for (uint8_t i = 0; i < WIRES_COUNT; i++) {
                if (mode_data.wires.wires[i].required) {
                    uint8_t lights = rng_generate8(&seed, WIRES_RNG_MASK);               
                    mode_data.wires.wires[i].light_left = ((lights & 0b01) == 0b01);
                    mode_data.wires.wires[i].light_right = ((lights & 0b10) == 0b10);
                }
            }
        }

        mode_data.wires.process.init_stage++;
    } else if (mode_data.wires.process.init_stage == 2 || mode_data.wires.process.init_stage == 3) {
        bool ready = true;

        for (uint8_t i = 0; i < WIRES_COUNT; i++) {
            if (mode_data.wires.wires[i].wiretype == NO_WIRE && mode_data.wires.wires[i].required) {
                ready = false;
                break;
            } else if (mode_data.wires.wires[i].wiretype != NO_WIRE && !mode_data.wires.wires[i].required) {
                ready = false;
                break;
            }
        }

        if (mode_data.wires.process.init_stage == 2 && ready) {
            if (mode_data.wires.complex) {
                wires_calculate_solution_complex();
            } else {
                wires_calculate_solution_simple();
            }

            game_module_ready(true);
            mode_data.wires.process.init_stage++;
        } else if (mode_data.wires.process.init_stage == 3 && !ready) {
            game_module_ready(false);
            mode_data.wires.process.init_stage--;
        }
    }

    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        if (mode_data.wires.wires[i].wiretype == NO_WIRE && mode_data.wires.wires[i].required) {
            if (mode_data.wires.process.animation_tick == 0) {
                argb_set(1 + i, 31, 0x00, 0xff, 0x00);
                argb_set(7 + i, 31, 0x00, 0xff, 0x00);
            } else if (mode_data.wires.process.animation_tick == 5) {
                argb_set(1 + i, 31, 0x00, 0x00, 0x00);
                argb_set(7 + i, 31, 0x00, 0x00, 0x00);
            }
        } else if (mode_data.wires.wires[i].wiretype != NO_WIRE && !mode_data.wires.wires[i].required) {
            if (mode_data.wires.process.animation_tick == 0) {
                argb_set(1 + i, 31, 0xff, 0x00, 0x00);
                argb_set(7 + i, 31, 0xff, 0x00, 0x00);
            } else if (mode_data.wires.process.animation_tick == 5) {
                argb_set(1 + i, 31, 0x00, 0x00, 0x00);
                argb_set(7 + i, 31, 0x00, 0x00, 0x00);
            }
        } else {
            uint8_t j = mode_data.wires.wires[i].wiretype;
            uint8_t p = wirelookup[j].colours.plug;
            uint8_t w = wirelookup[j].colours.wire;

            argb_set(1 + i, 31, wire_colors[p][0], wire_colors[p][1], wire_colors[p][2]);
            argb_set(7 + i, 31, wire_colors[w][0], wire_colors[w][1], wire_colors[w][2]);
        }
    }


    mode_data.wires.process.animation_tick++;
    if (mode_data.wires.process.animation_tick >= 9) {
        mode_data.wires.process.animation_tick = 0;
    }
}

void wires_calculate_solution_simple(void) {
    uint8_t wire_count = wires_get_wire_count();
    bool lastSerialDigitIsOdd = (edgework_serial_last_digit() % 2 == 1);
    
    switch(wire_count) {
        case 3:
            /*
             If there are no red wires, cut the second wire.
             Otherwise, if the last wire is white, cut the last wire.
             Otherwise, if there is more than one blue wire, cut the last blue wire.
             Otherwise, cut the last wire. 
             */
            if (wires_get_color_count(WIRE_COLOUR_RED) == 0) {
                wires_set_cut(2);
            } else if (wires_get_last_wire_color() == WIRE_COLOUR_WHITE) {
                wires_set_cut(-1);
            } else if (wires_get_color_count(WIRE_COLOUR_BLUE) > 1) {
                wires_set_cut_color(WIRE_COLOUR_BLUE, -1);
            } else {
                wires_set_cut(-1);
            }
            break;
        case 4:                        
            /*
             If there is more than one red wire and the last digit of the serial number is odd, cut the last red wire.
             Otherwise, if the last wire is yellow and there are no red wires, cut the first wire.
             Otherwise, if there is exactly one blue wire, cut the first wire.
             Otherwise, if there is more than one yellow wire, cut the last wire.
             Otherwise, cut the second wire. 
             */
            if (wires_get_color_count(WIRE_COLOUR_RED) > 1 && lastSerialDigitIsOdd) {
                wires_set_cut_color(WIRE_COLOUR_RED, -1);
            } else if (wires_get_last_wire_color() == WIRE_COLOUR_YELLOW && wires_get_color_count(WIRE_COLOUR_RED) == 0) {
                wires_set_cut(1);
            } else if (wires_get_color_count(WIRE_COLOUR_BLUE) == 1) {
                wires_set_cut(1);
            } else if(wires_get_color_count(WIRE_COLOUR_YELLOW) > 1) {
                wires_set_cut(-1);                
            } else {
                wires_set_cut(2);                
            }            
            break;
        case 5:
            /*
             If the last wire is black and the last digit of the serial number is odd, cut the fourth wire.
             Otherwise, if there is exactly one red wire and there is more than one yellow wire, cut the first wire.
             Otherwise, if there are no black wires, cut the second wire.
             Otherwise, cut the first wire. 
             */
            if (wires_get_last_wire_color() == WIRE_COLOUR_BLACK && lastSerialDigitIsOdd) {
                wires_set_cut(4);
            } else if (wires_get_color_count(WIRE_COLOUR_RED) == 1 && wires_get_color_count(WIRE_COLOUR_YELLOW) > 1) {
                wires_set_cut(1);               
            } else if (wires_get_color_count(WIRE_COLOUR_BLACK) == 0) {
                wires_set_cut(2);               
            } else {
                wires_set_cut(1);                               
            }
            break;
        case 6:
            /*
             If there are no yellow wires and the last digit of the serial number is odd, cut the third wire.
             Otherwise, if there is exactly one yellow wire and there is more than one white wire, cut the fourth wire.
             Otherwise, if there are no red wires, cut the last wire.
             Otherwise, cut the fourth wire. 
             */
            if (wires_get_color_count(WIRE_COLOUR_YELLOW) == 0 && lastSerialDigitIsOdd) {
                wires_set_cut(3);           
            } else if (wires_get_color_count(WIRE_COLOUR_YELLOW) == 1 && wires_get_color_count(WIRE_COLOUR_WHITE) > 1) {
                wires_set_cut(4);
            } else if (wires_get_color_count(WIRE_COLOUR_RED) == 0) {
                wires_set_cut(-1);
            } else {
                wires_set_cut(4);                
            }
            break;            
    }
}

void wires_set_cut(int8_t wire) {
    uint8_t last = 0;
    uint8_t c = 0;
    
    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        if (mode_data.wires.wires[i].wiretype != NO_WIRE) {
            c++;
            last = i;
            if (wire == c) {
                mode_data.wires.wires[i].cutneeded = 1;
                return;
            }
        }
    }
    
    if (wire == -1) {
        mode_data.wires.wires[last].cutneeded = 1;
    }
}

void wires_set_cut_color(uint8_t color, int8_t wire) {
    uint8_t last = 0;
    uint8_t c = 0;
    
    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        if (mode_data.wires.wires[i].wiretype != NO_WIRE) {
            if (wirelookup[mode_data.wires.wires[i].wiretype].colours.wire == color) {                       
                c++;
                last = i;
                if (wire == c) {
                    mode_data.wires.wires[i].cutneeded = 1;
                    return;
                }
            }
        }
    }
    
    if (wire == -1) {
        mode_data.wires.wires[last].cutneeded = 1;
    } 
}

uint8_t wires_get_wire_count(void) {
    uint8_t c = 0;
    
    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        if (mode_data.wires.wires[i].wiretype != NO_WIRE) {
            c++;
        }
    }
    
    return c;
}

uint8_t wires_get_color_count(uint8_t color) {
    uint8_t c = 0;
    
    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        uint8_t wt = mode_data.wires.wires[i].wiretype;
        
        if (wirelookup[wt].colours.plug == color || wirelookup[wt].colours.wire == color) {
            c++;            
        }
    }
    
    return c;
}

uint8_t wires_get_last_wire_color(void) {
    uint8_t last = 0;
    
    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        uint8_t wt = mode_data.wires.wires[i].wiretype;
        
        if (wt != NO_WIRE) {
            last = wirelookup[wt].colours.wire;
        }
    }
    
    return last;
}

void wires_calculate_solution_complex(void) {
    
}

/**
 * Service the module during the starting phase.
 * 
 * @param first true if first time module has been called
 */
void wires_service_start(bool first) {
    if (first) {
        for (uint8_t i = 0; i < WIRES_COUNT; i++) {
            if (mode_data.wires.wires[i].light_left) {
                argb_set(1 + i, 31, 0xff, 0xff, 0xff);
            } else {
                argb_set(1 + i, 31, 0, 0, 0);
            }

            if (mode_data.wires.wires[i].light_right) {
                argb_set(7 + i, 31, 0xff, 0xff, 0xff);
            } else {
                argb_set(7 + i, 31, 0, 0, 0);
            }

            mode_data.wires.wires[i].updated = false;
        }
    }
}

/**
 * Service the module during the running phase.
 * 
 * @param first true if first timed called
 */
void wires_service_running(bool first) {
    if (this_module->solved) {
        return;
    }

    wires_service_adc();

    uint8_t cuts_remaining = 0;

    for (uint8_t i = 0; i < WIRES_COUNT; i++) {
        if (mode_data.wires.wires[i].updated == 1) {
            mode_data.wires.wires[i].updated = 0;

            /* If a new wire was added illegally, or it was CUT incorrectly. */
            if (mode_data.wires.wires[i].wiretype != NO_WIRE || !mode_data.wires.wires[i].cutneeded) {
                game_module_strike(1);
            }
        }

        if (mode_data.wires.wires[i].cutneeded && mode_data.wires.wires[i].wiretype != NO_WIRE) {
            cuts_remaining++;
        }
    }

    if (cuts_remaining == 0) {
        game_module_solved(true);
    }
}

void wires_service_adc(void) {
    if (ADCON0bits.GO) {
        return;
    }

    if (mode_data.wires.process.adc_check != 0xff) {
        uint16_t adc_result = (ADRESH << 8) | ADRESL;

        mode_data.wires.wires[mode_data.wires.process.adc_check].adc_value = adc_result;

        uint8_t new_wire = 0xff;

        for (uint8_t i = 0; i < WIRE_POSSIBLE_COUNT; i++) {
            if (adc_result <= wirelookup[i].adc) {
                new_wire = i;
                break;
            }
        }

        if (mode_data.wires.wires[mode_data.wires.process.adc_check].wiretype != new_wire) {
            mode_data.wires.wires[mode_data.wires.process.adc_check].updated = 1;
            mode_data.wires.wires[mode_data.wires.process.adc_check].wiretype = new_wire;
        }

        mode_data.wires.process.adc_check++;
        if (mode_data.wires.process.adc_check >= WIRES_COUNT) {
            mode_data.wires.process.adc_check = 0;
            if (mode_data.wires.process.init_stage == 0) {
                mode_data.wires.process.init_stage = 1;
            }
        }
    } else {
        mode_data.wires.process.adc_check = 0;
    }

    /* Start an ADC conversion. Because KPORTB maps to normal PORTF, and PORTF starts at 0b101000 in the ADC table, we can just add that to the check number. */
    ADPCH = 0b101000 + mode_data.wires.process.adc_check;
    ADCON0bits.GO = 1;
}