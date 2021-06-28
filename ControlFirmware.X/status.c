#include <xc.h>
#include "argb.h"
#include "status.h"
#include "tick.h"

/* Colours for different status modes. */
const uint8_t led_colors[8][3] = {
    { 0xff, 0xff, 0x00 },  
    { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0x00, 0xff },
    { 0x00, 0x00, 0x00 }
};

/* Current status. */
uint8_t current_status = STATUS_READY;
/* Current error. */
uint8_t current_error = ERROR_NONE;

/* Phase in error animation. */
uint8_t error_phase = 0;
/* Next tick to update error animation. */
uint24_t error_last_tick = 0;

/**
 * Set the status indicator to the new mode.
 * 
 * @param status new status
 */
void status_set(uint8_t status) {
    current_status = status;
    
    if (current_error == ERROR_NONE) {
        argb_set(0, 31, led_colors[current_status][0], led_colors[current_status][1], led_colors[current_status][2]);
    }
}

/**
 * Set the status indicator to an animated error mode.
 * 
 * @param error error mode
 */
void status_error(uint8_t error) {
    current_error = error;
    
    if (current_error != ERROR_NONE) {
        argb_set(0, 31, 0, 0, 0);
        error_phase = 0;
    }   
}

/**
 * Service the status indicator and animate error if needed.
 */
void status_service(void) {
    if (current_error == ERROR_NONE) {
        return;
    }
    
    uint24_t now = tick_fetch();
    if ((now - error_last_tick) < 20) {
        return;
    }
    
    error_last_tick = now;
    
    switch(error_phase) {
        case 0:
            argb_set(0, 31, led_colors[current_status][0], led_colors[current_status][1], led_colors[current_status][2]);
            break;
        case 1:
            argb_set(0, 31, 0, 0, 0);
            break;            
        case 2:
            argb_set(0, 31, 255, 0, 0);
            break;
        case 3:
            argb_set(0, 31, 0, 0, 0);
            break;
        case 4:
            argb_set(0, 31, 255, 0, 0);
            break;
        case 5:
            argb_set(0, 31, 0, 0, 0);
            break;        
        case 6:
            if (current_error == ERROR_LOCAL) {
                argb_set(0, 31, 0, 255, 0);
            } else {
                argb_set(0, 31, 0, 0, 255);
            }            
            break;     
        case 7:
            argb_set(0, 31, 0, 0, 0);
            break;              
    }
        
    error_phase++;
    if (error_phase == 11) {
        error_phase = 0;
    }
}
