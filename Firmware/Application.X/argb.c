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
#include "hal/pins.h"
#include "hal/spi.h"
#include "argb.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"

/* Allocate the default ARGB buffers. */
argb_led_t ARGB_DEFAULT_LEDS[ARGB_MODULE_COUNT(0)];
uint8_t ARGB_DEFAULT_OUTPUT[ARGB_BUFFER_SIZE(0)];

/* Use the default LEDs. */
uint8_t argb_brightness;
uint8_t argb_count = 0;
bool argb_dirty = false;
bool argb_spi_queued = false;
argb_led_t *argb_leds = NULL;
uint8_t *argb_buffer = NULL;


/* Local function prototypes. */
spi_command_t *argb_spi_callback(spi_command_t *);

spi_device_t argb_spi_device = {
    .clk_pin = KPIN_SYS_B3,
    .miso_pin = KPIN_NONE,
    .mosi_pin = KPIN_SYS_B2,
    .cs_pin = KPIN_NONE,
    .baud = SPI_BAUD_1600_KHZ,
    .cke = true,
};

spi_command_t argb_spi_cmd;

/**
 * Initialise the port and configure SPI ready for ARGB.
 */
void argb_initialise(void) {
    /* Load last brightness from EEPROM. */
    argb_brightness = nvm_eeprom_read(EEPROM_LOC_ARGB_BRIGHTNESS);

    /* Configure SPI pins to output mode. */
    TRISBbits.TRISB2 = 0;
    TRISBbits.TRISB3 = 0;

    argb_spi_cmd.device = &argb_spi_device;
    argb_spi_cmd.operation = SPI_OPERATION_WRITE;
    argb_spi_cmd.callback = spi_unused_callback;

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

    argb_spi_cmd.buffer = argb_buffer;
    argb_spi_cmd.write_size = ARGB_BUFFER_SIZE(count);
    argb_spi_cmd.callback = argb_spi_callback;

    /* Mark it dirty so its sent. */
    argb_dirty = true;
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

uint8_t argb_get_brightness(void) {
    return argb_brightness;
}

void argb_set_brightness(uint8_t new_bri) {
    argb_brightness = new_bri & 0b00011111;
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

    if (argb_spi_queued) {
        return;
    }

    /* Copy data from the LED buffer into the SPI buffer, and mark as clean. */
    for (uint8_t i = 0; i < argb_count; i++) {
        uint8_t b = 4 + (i * 4);

        /* First bit of LED must be 1, keep other 2 unused bits 1 as well. */
        argb_buffer[b] = 0b11100000 | argb_brightness;
        argb_buffer[b + 1] = argb_leds[i].b;
        argb_buffer[b + 2] = argb_leds[i].g;
        argb_buffer[b + 3] = argb_leds[i].r;
    }

    spi_enqueue(&argb_spi_cmd);

    /* Mark as SPI in flight. */
    argb_spi_queued = true;

    /* Mark buffer as clean.*/
    argb_dirty = false;
}

spi_command_t *argb_spi_callback(spi_command_t *cmd) {
    argb_spi_queued = false;
    return NULL;
}