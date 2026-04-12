#ifndef ARGB_H
#define ARGB_H

#include <stdint.h>
#include <stdbool.h>
#include "pin.h"

typedef struct {
    uint8_t G;
    uint8_t R;
    uint8_t B;
} argb_led_t;

#define ARGB_DEFAULT_BUFFER_SIZE 1

/**
 * Initialize the ARGB driver output.
 *
 * @param out Output pin used for the LED data line.
 * @param negate True to invert signal polarity; false for normal polarity.
 */
void argb_init(pin_t out, bool negate);

/** Service ARGB state machine, must be invoked from the main loop context. */
void argb_service(void);

/** Handle interrupts relating to ARGB system.
 *
 * Must be called from consumer, take note to update irq vector to match the
 * configured ARGB_SPI_PERIPHERAL macro.
 *
 * void __interrupt(irq(IRQ_SPI1), base(0x208)) int_argb(void) {
 *   argb_interrupt();
 * }
 */
void argb_interrupt(void);

/**
 * Set the LED buffer used by the driver, this allows consumers to specify
 * a buffer suitable for their needs.
 *
 * The buffer size must always be larger than ARGB_DEFAULT_BUFFER_SIZE.
 *
 * @param buffer Pointer to new LED buffer.
 * @param len Number of LEDs in @p buffer.
 */
void argb_set_buffer(argb_led_t *buffer, uint8_t len);

/**
 * Set one LED color in the active buffer.
 *
 * @param idx Zero-based LED index to update.
 * @param r Red channel intensity (0-255).
 * @param g Green channel intensity (0-255).
 * @param b Blue channel intensity (0-255).
 */
void argb_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);

/** Queue transmission of the current buffer to the LED chain. */
void argb_update(void);

#endif
