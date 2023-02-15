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

/* Allocate the default ARGB buffers. */
argb_led_t ARGB_DEFAULT_LEDS[ARGB_MODULE_COUNT(0)];
uint8_t    ARGB_DEFAULT_OUTPUT[ARGB_BUFFER_SIZE(0)];

/* Use the default LEDs. */
uint8_t argb_brightness = 0b11111;
uint8_t argb_count = 0;
bool argb_dirty = false;
argb_led_t *argb_leds = NULL;
uint8_t *argb_buffer = NULL;

/**
 * Initialise the port and configure SPI ready for ARGB.
 */
void argb_initialise(void) {
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

    /* Set destination address of data. */
    DMAnDSA = &SPI1TXB;

    /* Set source address to general purpose register space. */
    DMAnCON1bits.SMR = 0b00;

    /* Increment source address after every transfer. */
    DMAnCON1bits.SMODE = 0b01;
    /* Keep destination address static after every transfer. */
    DMAnCON1bits.DMODE = 0b00;

    /* Set destination sizes, source to the size of the buffer. */
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
    
    /* Init ARGB with default buffers. */
    argb_expand(0, &ARGB_DEFAULT_LEDS[0], &ARGB_DEFAULT_OUTPUT[0]);
}

void argb_expand(uint8_t count, argb_led_t *leds, uint8_t *output) {
    /* Store references to buffers. */
    argb_leds = leds;
    argb_buffer = output;
    
    /* Add the status LED onto our count. */
    argb_count = count + 1;
    
    /* Clear buffer memory, may have LED state from previous boot. */
    for (uint8_t i = 0; i < ARGB_MODULE_COUNT(count); i++) {
        argb_leds[i].r = 0;
        argb_leds[i].g = 0;
        argb_leds[i].b = 0;
    }

    /* Initialise SPI buffer, 32 0 bits, and the rest 1 bits. */
    argb_buffer[0] = 0x00;
    argb_buffer[1] = 0x00;
    argb_buffer[2] = 0x00;
    argb_buffer[3] = 0x00;

    for (uint8_t i = 4; i < ARGB_BUFFER_SIZE(count); i++) {
        argb_buffer[i] = 0xff;
    }
    
    /* Mark it dirty so its send. */
    argb_dirty = true;
    
    /* Select DMA1 for configuration. */
    DMASELECT = 0;
    
    /* Disable DMA module. */
    DMAnCON0bits.EN = 0;
    
    /* Set source and size of output buffer. */
    DMAnSSA = &argb_buffer[0];
    DMAnSSZ = ARGB_BUFFER_SIZE(count);
    
    /* Enable DMA module. */
    DMAnCON0bits.EN = 1;
}


/**
 * Set an LEDs output on module.
 *
 * @param led led number to set, 0 is first module LED.
 * @param r red value
 * @param g green value
 * @param b blue value
 */
void argb_set_module(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    led++;
    
    if (led > argb_count) {
        return;
    }

    argb_leds[led].r = r;
    argb_leds[led].g = g;
    argb_leds[led].b = b;
    argb_dirty = true;
}

/**
 * Set an LEDs output for the status LED.
 *
 * @param r red value
 * @param g green value
 * @param b blue value
 */
void argb_set_status(uint8_t r, uint8_t g, uint8_t b) {
    argb_leds[0].r = r;
    argb_leds[0].g = g;
    argb_leds[0].b = b;
    argb_dirty = true;
}

/**
 * Check to see if a new frame is available for ARGB LEDs, and initiate an SPI
 * transfer if so and DMA is idle.
 */
void argb_service(void) {
    /* Check to see if update is needed. */
    if (!argb_dirty) {
        return;
    }

    /* Check to see if a DMA transfer is in progress (i.e. SPI is busy). */
    DMASELECT = 0;
    if (DMAnCON0bits.DGO == 1) {
        return;
    }

    /* Copy data from the LED buffer into the SPI buffer, and mark as clean. */
    for (uint8_t i = 0; i < argb_count; i++) {
        uint8_t b = 4 + (i*4);

        /* First bit of LED must be 1, keep other 2 unused bits 1 as well. */
        argb_buffer[b] = 0b11100000 | argb_brightness;
        argb_buffer[b+1] = argb_leds[i].b;
        argb_buffer[b+2] = argb_leds[i].g;
        argb_buffer[b+3] = argb_leds[i].r;
    }

    /* Mark buffer as clean.*/
    argb_dirty = false;

    /* Start SPI DMA, ensure that length is correct. */
    DMAnSSZ = ARGB_BUFFER_SIZE(argb_count - 1);
    DMAnCON0bits.SIRQEN = 1;
}