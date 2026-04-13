#include <xc.h>
#include <hal/mcu.h>
#include <hal/interrupt.h>
#include <hal/i2c.h>
#include <hal/spi.h>
#include <hal/argb.h>
#include <utils/time.h>
#include "gpio.h"

/* Main entry point for control board. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode, doze don't sleep. */
  sleep_doze();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  /* Initialise time. */
  time_init();

  /* Initialise I2C. */
  i2c_init(GPIO_SCL, GPIO_SDA);

  /* Initialise SPI. */
  spi_init(GPIO_SPI_COPI, GPIO_SPI_CLK, GPIO_SPI_CIPO);

  /* Intialise ARGB */
  argb_init(GPIO_ARGB, false);

  /* Main loop. */
  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Service I2C. */
    i2c_service();

    /* Service SPI. */
    spi_service();

    /* Service ARGB. */
    argb_service();

    /* Sleep, if there's no time update. */
    if (time_service_end()) {
      SLEEP();
    }
  }

  return 0;
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default),base(0x208)) int_default(void) {
}

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}

/* I2C and it's DMA. */
void __interrupt(irq(IRQ_I2C1, IRQ_I2C1E, IRQ_DMA2DCNT), base(0x208)) int_i2c(void) {
  i2c_interrupt();
}

/* SPI and it's DMA. */
void __interrupt(irq(IRQ_SPI2,IRQ_DMA1DCNT), base(0x208)) int_spi(void) {
  spi_interrupt();
}

/* ARGB's SPI. */
void __interrupt(irq(IRQ_SPI1), base(0x208)) int_argb(void) {
  argb_interrupt();
}
