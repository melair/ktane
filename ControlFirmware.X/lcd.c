#include <xc.h>
#include <stdbool.h>
#include "lcd.h"
#include "nvm.h"
#include "can.h"
#include "mode.h"

/* The LCD driver uses the XC8 delay routines, to init and to time E clock
 * pulses, this results in the main loop stopping but vastly simplifies control
 * flow. */
#define _XTAL_FREQ 64000000

/* Default PWM duty for brightness, from EEPROM. */
uint8_t lcd_default_brightness;
/* Default PWM duty for contrast, from EEPROM. */
uint8_t lcd_default_contrast;

/* Current PWM duty for brightness. */
uint8_t lcd_current_brightness;
/* Current PWM duty for contrast. */
uint8_t lcd_current_contrast;

/* Local function prototypes. */
void lcd_start(void);

#define LCD_RS   LATEbits.LATE0
#define LCD_RW   LATEbits.LATE1
#define LCD_E    LATEbits.LATE2
#define LCD_DATA LATC

#define LCD_BUSY      PORTCbits.RC7
#define LCD_BUSY_TRIS TRISCbits.TRISC7

#define LCD_BUFFER_USER     0
#define LCD_BUFFER_DESIRED  1
#define LCD_BUFFER_SHADOW   2

#define BUFFER_COUNT 3
#define LINE_COUNT   4
#define COLUMN_COUNT 20

/* Triple buffer, frame used by user, frame for desired state, frame for target state. */
uint8_t buffer[BUFFER_COUNT][LINE_COUNT][COLUMN_COUNT];
/* True if the shadow buffer is dirty. */
bool shadow_dirty = false;

#define PHASE_IDLE                     0
#define PHASE_POS_SEND                 1
#define PHASE_POS_SEND_AWAIT           2
#define PHASE_DATA_SEND                3
#define PHASE_DATA_SEND_AWAIT          4

/* Current phase of sending to LCD display. */
uint8_t send_phase = PHASE_IDLE;

/* Current row being processed while syncing a dirty desired buffer to the LCD. */
uint8_t dirty_scan_row = 0;
/* Current col being processed while syncing a dirty desired buffer to the LCD. */
uint8_t dirty_scan_col = 0;
/* Position of the character being send to the LCD screen. */
uint8_t send_pos = 0x00;
/* Character being sent to LCD screen. */
uint8_t send_data = 0x00;

/**
 * Initialise the LCD driver, including PWM3 (P1 and P2) for brightness and
 * contrast control.
 */
void lcd_initialize(void) {
    /* Load LCD brightness and contrast from NVM, load into current. */
    lcd_default_brightness = nvm_read(EEPROM_LOC_LCD_BRIGHTNESS);
    lcd_default_contrast = nvm_read(EEPROM_LOC_LCD_CONTRAST);
    
    /* Initialise ports. */
    TRISBbits.TRISB5 = 0;
    TRISBbits.TRISB7 = 0;
    TRISC = 0x00;
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    
    /* Disable pull up on RB7. */
    ODCONBbits.ODCB7 = 1;
    
    /* Set RB4 to use PWM3SP1. */      
    RB5PPS = 0x1c;
    /* Set RB7 to use PWM3SP2. */   
    RB7PPS = 0x1d;
 
    /* Disable external reset. */
    PWM3ERS = 0;
    
    /* Set PWM clock to FSOC, 64MHz. */
    PWM3CLKbits.CLK = 0b00010;
    
    /* Disable auto load. */
    PWM3LDS = 1;
    
    /* Prescale to 1:128. */
    PWM3CPRE = 127; 
    
    /* PWM at ~1kHz. */
    PWM3PR = 512;
       
    /* Initial duty to zero.*/
    PWM3S1P1 = 0;
            
    /* Enable PWM. */
    PWM3CONbits.EN = 1;
    
    /* Set defaults. */
    lcd_set_brightness(lcd_default_brightness);
    lcd_set_contrast(lcd_default_contrast);     
    
    lcd_start();   
}

/**
 * Initialise the LCD screen, this is a blocking function using CPU spinning for
 * time delays. Needed as Busy flag can not be used until LCD is initialised.
 */
void lcd_start(void) {
    /* Initial driver start up delay. */
    __delay_ms(50);
    
    /* Initial configuration, 2 line, 5*8 font, 8 bit.*/
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_DATA = 0b00111000;
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;
    
    __delay_ms(1);
    
    /* Reset init. */
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;
       
    /* Reset init. */
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;
     
    __delay_ms(1);

    /* Set display on, cursor off. */
    LCD_DATA = 0b00001100;
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;
      
    __delay_ms(1);

    /* Clear display. */
    LCD_DATA = 0b00000001;
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;  

    __delay_ms(1);

    /* Set entry mode, increment, shift right.*/
    LCD_DATA = 0b00000110;
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;     
}

/**
 * Set the brightness of the LCD backlight.
 * 
 * @param bri 0-255 brightness
 */
void lcd_set_brightness(uint8_t bri) {
    lcd_current_brightness = bri;
    uint16_t duty = bri * 2;    

    PWM3S1P1 = duty;
    PWM3CONbits.LD = 1;
}

/**
 * Set the contract of the LCD display.
 * 
 * @param cont 0-255 contrast, lower numbers will be higher contrast
 */
void lcd_set_contrast(uint8_t cont) {
    lcd_current_contrast = cont;
    uint16_t duty = cont * 2;    

    PWM3S1P2 = duty;
    PWM3CONbits.LD = 1;
}

/**
 * Service the LCD, if the shadow buffer is dirty then find changes and send
 * them to the LCD display. Only changes are sent so single character changes
 * are very quick.
 * 
 * This routine does us spinning CPU delays of 1uS, this is because the E line
 * must be held high for at least 450nS.
 */
void lcd_service(void) {
    /* Check to see if the shadow buffer does not match the desired buffer. */
    if (!shadow_dirty) {
        return;
    }
    
    /* If we're currently not sending something try, find what needs changing. */
    if (send_phase == PHASE_IDLE) {
        uint8_t next_pos = 0x00;
        uint8_t next_data = 0xff;
        
        /* Loop through the desired buffer, checking the shadow to see if it matches.
         * This upon finding something to update, the current row/col are stored in 
         * the dirty_scan variables. This prevents us having to search through every-
         * thing again on the next service. */
        for (uint8_t row = dirty_scan_row; row < LINE_COUNT; row++) {
            for (uint8_t col = dirty_scan_col; col < COLUMN_COUNT; col++) {
                if (buffer[LCD_BUFFER_DESIRED][row][col] != buffer[LCD_BUFFER_SHADOW][row][col]) {
                    /* Capture the new character to display, update the shadow buffer
                     * to match what we're about to send. */
                    next_data = buffer[LCD_BUFFER_DESIRED][row][col];
                    buffer[LCD_BUFFER_SHADOW][row][col] = next_data;    

                    /* Calculate the position in DGRAM of the HD44780. */
                    next_pos = (0x40 * row) + col;
                    
                    /* Support displays with > 2 row by offsetting the position lower. */
                    if (row > 1) {
                        next_pos -= 0x6c;
                    }
                    
                    /* Store the scan position to shortcut next run. */
                    dirty_scan_row = row;
                    dirty_scan_col = col;
                    
                    break;
                }
            }
            
            /* Break the loop if we find anything to change. */
            if (next_data != 0xff) {
                 break;
            }
            
            /* Manually reset the column. */
            dirty_scan_col = 0;
        }
        
        /* If we found nothing at all, reset the scan and mark buffer clean. */
        if (next_data == 0xff) {
            dirty_scan_row = 0;
            dirty_scan_col = 0;
            shadow_dirty = false;
            return;
        }    

        /* If the character we're about to send is the next character from
         * the previous we sent, we don't need to send the position. */
        if (next_pos == send_pos + 1) {
            send_phase = PHASE_DATA_SEND;           
        } else {
            send_phase = PHASE_POS_SEND;
        }
                    
        send_pos = next_pos;
        send_data = next_data;        
    } 
    
    /* If we need to send the position. */
    if (send_phase == PHASE_POS_SEND) {
        LCD_BUSY_TRIS = 0;
        
        LCD_RS = 0;
        LCD_RW = 0;
        LCD_DATA = 0b10000000 | send_pos;
        LCD_E = 1;
        __delay_us(1);
        LCD_E = 0;
        LCD_DATA = 0x00;
           
        send_phase = PHASE_POS_SEND_AWAIT;
        return;
    }
    
    /* If we need to send the character data. */
    if (send_phase == PHASE_DATA_SEND) {
        LCD_BUSY_TRIS = 0;
        
        LCD_RS = 1;
        LCD_RW = 0;
        LCD_DATA = send_data;
        LCD_E = 1;
        __delay_us(1);
        LCD_E = 0;
        LCD_RS = 0;
        LCD_DATA = 0x00;
        
        send_phase = PHASE_DATA_SEND_AWAIT;
        return;
    }
    
    /* If we need to wait for the LCD busy signal, using the busy signal we can
     * avoid any spinning CPU for updating the CPU, or complex flow control. */
    if (send_phase == PHASE_POS_SEND_AWAIT || send_phase == PHASE_DATA_SEND_AWAIT) {
        LCD_BUSY_TRIS = 1;
        
        /* Checking the busy signal requires us to change to R mode and set E,
         * checking the BUSY flag while E is high. Busy lags E by a few nS. */
        LCD_RS = 0;
        LCD_RW = 1;
        LCD_DATA = 0x00;
        LCD_E = 1;
        __delay_us(1);
        
        /* If we're not busy, we can move onto the next phase. */
        if (!LCD_BUSY) {            
            if (send_phase == PHASE_POS_SEND_AWAIT) {
                /* If we sent the position, we now need to send the data. */
                send_phase = PHASE_DATA_SEND;
            } else {
                /* If we sent the data, we can move onto the next character. */
                send_phase = PHASE_IDLE;
            }
        }
        
        LCD_E = 0;
        LCD_RW = 0;
        
        /* We need to move TRIS back to OUTPUT, if we don't setting RW makes DB7/BUSY
         * high impedance, and our PIN would also be high impedance - meaning that
         * the busy signal may be high next time we read. Setting us to a low output
         * drains it. */
        LCD_BUSY_TRIS = 0;
    }    
}

/**
 * Wipe the user buffer, a sync must be requested.
 */
void lcd_clear(void) {
    for (uint8_t row = 0; row < LINE_COUNT; row++) {
        for (uint8_t col = 0; col < COLUMN_COUNT; col++) {
            buffer[LCD_BUFFER_USER][row][col] = ' ';
        }
    }
}

/**
 * Update the user buffer with new content.
 * 
 * @param row row of the LCD panel
 * @param col column in row to start content
 * @param size size of content
 * @param data actual content
 */
void lcd_update(uint8_t row, uint8_t col, uint8_t size, uint8_t *data) {
    for (uint8_t i = col, j = 0; i < COLUMN_COUNT && j < size; i++, j++) {
        buffer[LCD_BUFFER_USER][row][i] = data[j];
    }
}

/**
 * Sync the contests of the user buffer to the desired buffer and mark shadow
 * buffer as dirty.
 */
void lcd_sync(void) {
    for (uint8_t row = 0; row < LINE_COUNT; row++) {
        for (uint8_t col = 0; col < COLUMN_COUNT; col++) {
            buffer[LCD_BUFFER_DESIRED][row][col] = buffer[LCD_BUFFER_USER][row][col];
        }
    }
    
    shadow_dirty = true;
}

/**
 * Initialise the LCD screen with useful data, this will remain on screen unless
 * the LCD screen is used by the module for the game.
 */
void lcd_default(void) {    
    const uint8_t *welcome_first = "KTANE by Melair";
    const uint8_t *welcome_second_mode = "Mode:";
    const uint8_t *welcome_second_can = "CAN:";

    const uint8_t *hex_map = "0123456789ABCDEF";

    uint8_t can_id = can_get_id();
    uint8_t mode = mode_get();
    
    lcd_update(0, 0, 15, welcome_first);
    
    lcd_update(1, 0, 5, welcome_second_mode);
    lcd_update(1, 6, 1, &hex_map[(mode >> 4) & 0x0f]);
    lcd_update(1, 7, 1, &hex_map[mode & 0x0f]);   
    lcd_update(1, 9, 4, welcome_second_can);
    lcd_update(1, 14, 1, &hex_map[(can_id >> 4) & 0x0f]);
    lcd_update(1, 15, 1, &hex_map[can_id & 0x0f]);
    
    lcd_sync();
}