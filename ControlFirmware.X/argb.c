/**
 * Provide support for APA102/HD107S SPI compatible addressable LEDs. All
 * modules contain at least one for status, others may use them for display
 * purposes.
 * 
 * ARGBs are essentially double buffered, an Bri,R,G,B structure and an SPI
 * output byte array. This results in around 300 bytes of RAM used, if tight
 * this could be made single buffer but we would need to prevent changes to
 * LED state during SPI writes.
 * 
 * USES: SPI1, DMA1
 */
#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "argb.h"

/* Structure for storing the state of an ARGB LED. */
typedef struct {
    uint8_t brightness;
    uint8_t r;
    uint8_t g; 
    uint8_t b;
} argb_led_t;

/* Maximum number of ARGBs supported by a control board, effects RAM usage. */
/* 37 = Status + (6*6 Maze) */
#define ARGB_MAX_LED (1 + (6*6))
#define ARGB_BUFFER_SIZE (ARGB_MAX_LED * 4 + 8)

/* Frame buffer for ARGB LEDs, can be changed during SPI frame output. */
argb_led_t leds_buffer[ARGB_MAX_LED];
/* Number of LEDs redirect by module. */
uint8_t leds_count = 1;
/* If LEDs have had changes and an output is required. */
bool leds_dirty;

/* Buffer for writing to SPI, only updated by SPI start process. */
uint8_t led_spi_buffer[ARGB_BUFFER_SIZE];
/* Size of SPI buffer to write, including start and end frames. */
uint8_t led_spi_size = 9;

/**
 * Initialise the port and configure SPI ready for ARGB.
 */
void argb_initialise(void) {
    /* Clear buffer memory, may have LED state from previous boot. */
    for (uint8_t i = 0; i <= ARGB_MAX_LED; i++) {
        leds_buffer[i].brightness = 0;
        leds_buffer[i].r = 0;
        leds_buffer[i].g = 0;
        leds_buffer[i].b = 0;
    }
    
    /* Initialise SPI buffer, 32 0 bits, and the rest 1 bits. */
    led_spi_buffer[0] = 0x00;
    led_spi_buffer[1] = 0x00;
    led_spi_buffer[2] = 0x00;
    led_spi_buffer[3] = 0x00;
    
    for (uint8_t i = 4; i < ARGB_BUFFER_SIZE; i++) {
        led_spi_buffer[i] = 0xff;
    }
    
    argb_module_leds(36);
        
    /* Configure source for RB2/RB3 to be SPI1 SCK/SDO. */
    RB2PPS = 0x32; // DAT
    RB3PPS = 0x31; // CLK
    
    /* Configure to output mode. */
    TRISBbits.TRISB2 = 0;
    TRISBbits.TRISB3 = 0;

    /* Set SPI baud to ~2MHz. */
    SPI1CLK = 0b000000; /* 64MhHz clock. */
    SPI1BAUD = 16;      /* 2.13MHz SPI baud. */
    
    /* Set to TX only. */
    SPI1CON2bits.TXR = 1;
    SPI1CON2bits.RXR = 0;   
    
    /* Act as bus master. */
    SPI1CON0bits.MST = 1;

    /* Set BMODE to 1, counting in bytes. */    
    SPI1CON0bits.BMODE = 1;

    /* Enable SPI1 peripheral. */
    SPI1CON0bits.EN = 1;
    
    /* Select DMA1 for configuration. */
    DMASELECT = 0;
    
    /* Reset DMA1. */
    DMAnCON0 = 0;
    
    /* Set source and destination address of data. */
    DMAnSSA = &led_spi_buffer[0];
    DMAnDSA = &SPI1TXB;
    
    /* Set source address to general purpose register space. */
    DMAnCON1bits.SMR = 0b00;
          
    /* Increment source address after every transfer. */
    DMAnCON1bits.SMODE = 0b01;
    /* Keep destination address static after every transfer. */
    DMAnCON1bits.DMODE = 0b00; 
    
    /* Set source and destination sizes, source to the size of the buffer. */
    DMAnSSZ = led_spi_size;
    DMAnDSZ = 1;
    
    /* Set clearing of SIREQEN bit when source counter is reloaded, don't when
     * destination counter is reloaded. */
    DMAnCON1bits.SSTP = 1;
    DMAnCON1bits.DSTP = 0;
    
    /* Set start and abort IRQ triggers, SPI1TX and None, respectively. */
    DMAnSIRQ = 0x19;
    DMAnAIRQ = 0x00;
    
    /* Prevent hardware triggers starting DMA transfer. */
    DMAnCON0bits.SIRQEN = 0;
    
    /* Enable DMA module. */    
    DMAnCON0bits.EN = 1;
}

/**
 * Set the number of required ARGBs by the modules specific function, internally
 * account for status LED. Mark buffer as dirty to cause send of all.
 * 
 * @param module_led_count number of ARGB LEDs needed by module
 */
void argb_module_leds(uint8_t module_led_count) {
    leds_count = module_led_count + 1;
    led_spi_size = 4 + (leds_count * 4) + 4;
    leds_dirty = true;
}

/**
 * Set an LEDs output.
 * 
 * @param led led number to set 0 is status, 1+ are module LEDs
 * @param bri 0-31 global brightness setting
 * @param r red value
 * @param g green value
 * @param b blue value
 */
void argb_set(uint8_t led, uint8_t bri, uint8_t r, uint8_t g, uint8_t b) {
    if (led >= leds_count) {
        return;
    }
    
    leds_buffer[led].brightness = bri & 0x1f;
    leds_buffer[led].r = r;
    leds_buffer[led].g = g;
    leds_buffer[led].b = b;
    leds_dirty = true;
}

/**
 * Check to see if a new frame is available for ARGB LEDs, and initiate an SPI 
 * transfer if so and DMA is idle.
 */
void argb_service(void) {
    /* Check to see if update is needed. */
    if (!leds_dirty) {
        return;
    }
    
    /* Check to see if a DMA transfer is in progress (i.e. SPI is busy). */
    DMASELECT = 0;   
    if (DMAnCON0bits.DGO == 1) {
        return;
    }
    
    /* Copy data from the LED buffer into the SPI buffer, and mark as clean. */
    for (uint8_t i = 0; i < leds_count; i++) {
        uint8_t b = 4 + (i*4);
        
        /* First bit of LED must be 1, keep other 2 unused bits 1 as well. */
        led_spi_buffer[b] = 0b11100000 | leds_buffer[i].brightness;
        led_spi_buffer[b+1] = leds_buffer[i].b;
        led_spi_buffer[b+2] = leds_buffer[i].g;
        led_spi_buffer[b+3] = leds_buffer[i].r;
    }
    
    /* Mark buffer as clean.*/
    leds_dirty = false;
    
    /* Start SPI DMA, ensure that length is correct. */
    DMAnSSZ = led_spi_size;
    DMAnCON0bits.SIRQEN = 1;
}