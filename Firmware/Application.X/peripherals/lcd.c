#include <xc.h>
#include <stdbool.h>
#include "lcd.h"
#include "../../common/nvm.h"
#include "../../common/eeprom_addrs.h"

/* Big font adapter from: https://github.com/varind/LCDCustomLargeFont */

/* The LCD driver uses the XC8 delay routines, to init and to time E clock
 * pulses, this results in the main loop stopping but vastly simplifies control
 * flow. */
#define _XTAL_FREQ 64000000

/* Maximum PWM duty for brightness, from EEPROM. */
uint8_t lcd_nominal_brightness;
/* Default PWM duty for contrast, from EEPROM. */
uint8_t lcd_nominal_contrast;

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

#define BIG_FONT_CHARACTERS 8

const uint8_t big_font_characters[BIG_FONT_CHARACTERS][LCD_CHARACTER_ROWS] = {
    { // UB
        0b11111,
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
    },
    { // RT
        0b11100,
        0b11110,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111
    },
    { // LL
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b01111,
        0b00111
    },
    { // LB
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111,
        0b11111
    },
    { // LR
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11110,
        0b11100
    },
    { // UMB
        0b11111,
        0b11111,
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111
    },
    { // LMB
        0b11111,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b11111,
        0b11111
    },
    { // LT
        0b00111,
        0b01111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
    },
};

#define BIG_FONT_CHARACTER_COUNT    36

const uint8_t big_font[BIG_FONT_CHARACTER_COUNT][4] = {
    { // 0
        0x70, 0x1e,
        0x23, 0x4e,
    },
    { // 1
        0x01, 0xee,
        0xef, 0xee,
    },
    { // 2
        0x55, 0x1e,
        0x26, 0x6e,
    },
    { // 3
        0x55, 0x1e,
        0x66, 0x4e,
    },
    { // 4
        0x23, 0x1e,
        0xee, 0xfe,
    },
    { // 5
        0xf5, 0x5e,
        0x66, 0x4e,
    },
    { // 6
        0x75, 0x5e,
        0x26, 0x4e,
    },
    { // 7
        0x00, 0x1e,
        0xe7, 0xee,
    },
    { // 8
        0x75, 0x1e,
        0x25, 0x4e,
    },
    { // 9
        0x75, 0x1e,
        0xee, 0xfe,
    },
    { // A
        0x75, 0x1e,
        0xfe, 0xfe,
    },
    { // B
        0xf5, 0x4e,
        0xf6, 0x1e,
    },
    { // C
        0x70, 0x0e,
        0x23, 0x3e,
    },
    { // D
        0xf0, 0x1e,
        0xf3, 0x4e,
    },
    { // E
        0xf5, 0x5e,
        0xf6, 0x6e,
    },
    { // F
        0xf5, 0x5e,
        0xfe, 0xee,
    },
    { // G
        0x70, 0x0e,
        0x23, 0x1e,
    },
    { // H
        0xf3, 0xfe,
        0xfe, 0xfe,
    },
    { // I
        0x0f, 0x0e,
        0x3f, 0x3e,
    },
    { // J
        0xee, 0xfe,
        0x33, 0x4e,
    },
    { // K
        0xf3, 0x4e,
        0xfe, 0x1e,
    },
    { // L
        0xfe, 0xee,
        0xf3, 0x3e,
    },
    { // M
        0x72, 0x41,
        0xfe, 0xef,
    },
    { // N
        0x71, 0xef,
        0xfe, 0x24,
    },
    { // O
        0x70, 0x1e,
        0x23, 0x4e,
    },
    { // P
        0xf5, 0x1e,
        0xfe, 0xee,
    },
    { // Q
        0x70, 0x1e,
        0x23, 0xf3,
    },
    { // R
        0xf5, 0x1e,
        0xfe, 0x1e,
    },
    { // S
        0x75, 0x5e,
        0x66, 0x4e,
    },
    { // T
        0x0f, 0x0e,
        0xef, 0xee,
    },
    { // U
        0xfe, 0xfe,
        0x23, 0x4e,
    },
    { // V
        0x2e, 0xe4,
        0xe1, 0x7e,
    },
    { // W
        0xfe, 0xef,
        0x27, 0x14,
    },
    { // X
        0x23, 0x4e,
        0x7e, 0x1e,
    },
    { // Y
        0x23, 0x4e,
        0xef, 0xee,
    },
    { // Z
        0x05, 0x4e,
        0x76, 0x3e,
    },
};

/**
 * Initialise the LCD driver, including PWM3 (P1 and P2) for brightness and
 * contrast control.
 */
void lcd_initialize(void) {
    /* Load LCD brightness and contrast from NVM, load into current. */
    lcd_nominal_brightness = nvm_eeprom_read(EEPROM_LOC_LCD_BRIGHTNESS);
    lcd_nominal_contrast = nvm_eeprom_read(EEPROM_LOC_LCD_CONTRAST);

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

    /* Prescale to 1:31. */
    PWM3CPRE = 15;

    /* PWM at ~8kHz. */
    PWM3PR = 512;

    /* Initial duty to zero.*/
    PWM3S1P1 = 0;

    /* Enable PWM. */
    PWM3CONbits.EN = 1;

    /* Set defaults. */
    lcd_set_brightness(lcd_nominal_brightness);
    lcd_set_contrast(lcd_nominal_contrast);

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
 * Load the 2x4 font into the character ram in the LCD display. This will block
 * execution, it is assumed it will be done during initialisation.
 */
void lcd_load_big(void) {
    for (uint8_t c = 0; c < BIG_FONT_CHARACTERS; c++) {
        lcd_custom_character(c, &big_font_characters[c]);
    }
}

/* Load a customer character into character RAM in the LCD display. This will
 * block execution, it is assumed it will be done during initialisation.
 */
void lcd_custom_character(uint8_t c, uint8_t *data) {
    LCD_RW = 0;

    /* Move pointer to Character Generator RAM for character. */
    LCD_RS = 0;
    LCD_DATA = 0b01000000 | (c << 3);
    LCD_E = 1;
    __delay_us(1);
    LCD_E = 0;

    __delay_ms(1);

    for (uint8_t l = 0; l < LCD_CHARACTER_ROWS; l++) {
        LCD_RS = 1;
        LCD_DATA = data[l];
        LCD_E = 1;
        __delay_us(1);
        LCD_E = 0;

        __delay_ms(1);
    }

    LCD_RS = 0;
}

/**
 * Get the nominal brightness.
 *
 * @return nominal brightness
 */
uint8_t lcd_get_nominal_brightness(void) {
    return lcd_nominal_brightness;
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
 * Get the nominal contrast.
 *
 * @return nominal contrast
 */
uint8_t lcd_get_nominal_contrast(void) {
    return lcd_nominal_contrast;
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
        uint8_t next_data = 0x00;
        bool found_data = false;

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
                    found_data = true;

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
            if (found_data) {
                break;
            }

            /* Manually reset the column. */
            dirty_scan_col = 0;
        }

        /* If we found nothing at all, reset the scan and mark buffer clean. */
        if (!found_data) {
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

const uint8_t *hex_map = "0123456789ABCDEF";

/**
 * Print a number to the LCD screen.
 *
 * @param row row of the LCD panel
 * @param col column in row to start number
 * @param digits number of digits
 * @param number number to display
 */
void lcd_number(uint8_t row, uint8_t col, uint8_t digits, uint16_t number) {
    for (uint8_t i = 0; i < digits; i++) {
        uint8_t remain = number % 10;
        number = (number - remain) / 10;

        lcd_update(row, (col + digits)-(i + 1), 1, &hex_map[remain]);
    }
}

/**
 * Print a hex number to the LCD screen.
 *
 * @param row row of the LCD panel
 * @param col column in row to start number
 * @param number number to display
 */
void lcd_hex(uint8_t row, uint8_t col, uint8_t number) {
    lcd_update(row, col, 1, &hex_map[(number >> 4) & 0x0f]);
    lcd_update(row, col + 1, 1, &hex_map[number & 0x0f]);
}

/**
 * Use 4x2 font to display a character, on a 20x4 screen.
 *
 * @param pos character position
 * @param ch character to display
 */
void lcd_update_big(uint8_t pos, uint8_t ch) {
    uint8_t t[8];

    t[0] = (big_font[ch][0] >> 4) & 0x0f;
    t[1] = big_font[ch][0] & 0x0f;
    t[2] = (big_font[ch][1] >> 4) & 0x0f;
    t[3] = big_font[ch][1] & 0x0f;
    t[4] = (big_font[ch][2] >> 4) & 0x0f;
    t[5] = big_font[ch][2] & 0x0f;
    t[6] = (big_font[ch][3] >> 4) & 0x0f;
    t[7] = big_font[ch][3] & 0x0f;

    for (uint8_t i = 0; i < 8; i++) {
        switch (t[i]) {
            case 0b1110:
                t[i] = 0xfe;
                break;
            case 0b1111:
                t[i] = 0xff;
                break;
        }
    }

    lcd_update(1, 4 * pos, 4, &t[0]);
    lcd_update(2, 4 * pos, 4, &t[4]);
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
