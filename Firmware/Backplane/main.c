#include <xc.h>
#include <hal/interrupt.h>
#include <hal/mcu.h>
#include <hal/pin.h>
#include <utils/time.h>
#include "gpio.h"
#include "power.h"
#include <hal/i2c.h>

/* Main entry point for backplane. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode. */
  sleep_doze();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  /* Initialise time. */
  time_init();

  /* Initialise I2C. */
  i2c_init(GPIO_SCL, GPIO_SDA);

  /* Initialise power manager. */
 // power_init();

  /* Main loop. */
  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Service I2C. */
    i2c_service();

    /* Service power manager. */
  //  power_service();

    /* Sleep, if there's no time update. */
    if (time_service_end()) {
      SLEEP();
    }
  }
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default), base(0x208)) int_default(void) {}

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}

/* I2C and it's DMA. */
void __interrupt(irq(IRQ_I2C1, IRQ_I2C1E, IRQ_DMA2DCNT), base(0x208)) int_i2c(void) {
  i2c_interrupt();
}