#include "gpio.h"
#include <hal/argb.h>
#include <hal/dma.h>
#include <hal/i2c.h>
#include <hal/interrupt.h>
#include <hal/mcu.h>
#include <hal/pin.h>
#include <hal/spi.h>
#include <language_support.h>
#include <peripherals/epaper/epaper.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils/time.h>
#include <xc.h>

/* Main entry point for edgework widget. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode. */
  sleep_doze();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  /* Initialise time. */
  time_init();

  /* Initialise I2C */
  i2c_init(GPIO_SCL, GPIO_SDA);

  /* Temp: Intialise ARGB */
  argb_init(GPIO_0, false);

  argb_set(0, 0x00, 0xff, 0xff);

  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Cause ARGB to output. */
    if (tick_20hz) {
      argb_set(0, 0x00, uptime_in_ms & 0xff, (0xff - (uptime_in_ms & 0xff)));
      argb_update();
    }

    /* Service I2C. */
    i2c_service();

    /* Service ARGB */
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
void __interrupt(irq(default), base(0x208)) int_default(void) {}

/* I2C and it's DMA. */
void __interrupt(irq(IRQ_I2C1, IRQ_I2C1E, IRQ_DMA2DCNT), base(0x208))
    int_i2c(void) {
  i2c_interrupt();
}

/* ARGB's SPI. */
void __interrupt(irq(IRQ_SPI1), base(0x208)) int_argb(void) {
  argb_interrupt();
}

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}
