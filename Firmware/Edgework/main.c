#include <language_support.h>
#include <xc.h>
#include <hal/mcu.h>
#include <hal/interrupt.h>
#include <hal/dma.h>
#include <hal/spi.h>
#include "gpio.h"
#include "hal/pin.h"

spi_t spi;

/* Main entry point for edgework widget. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode. */
  sleep_init();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  /* Temp: Initialise SPI. */
  pin_config(GPIO_4, OUTPUT, 0); // COPI
  pin_config(GPIO_5, OUTPUT, 0); // CLK
  pin_config(GPIO_6, OUTPUT, 0); // /CS
  pin_write(GPIO_6, true);

  spi_init(&spi, GPIO_4, GPIO_5, PORTPIN_NONE, SPI1 | DMA1);

  while(1) {
    // Clear Watchdog.
    CLRWDT();

    /* Temp: Service SPI. */
    spi_service(&spi);

    // Sleep.
    SLEEP();
  }

  return 0;
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default), base(0x208)) int_default(void) {
}

/* Temp: SPI Interrupt routing. */
void __interrupt(irq(0x1A), base(0x208)) int_spi1(void) {
  spi_interrupt(&spi);
}