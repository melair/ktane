#include <language_support.h>
#include <stdint.h>
#include <xc.h>
#include <hal/mcu.h>
#include <hal/interrupt.h>
#include <hal/dma.h>
#include <hal/spi.h>
#include "gpio.h"
#include <hal/pin.h>
#include <utils/time.h>

/* Main entry point for edgework widget. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode. */
  sleep_init();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  /* Initialise time. */
  time_init();

  /* Temp: Initialise SPI. */
  pin_config(GPIO_4, OUTPUT, 0); // COPI
  pin_config(GPIO_5, OUTPUT, 0); // CLK
  pin_config(GPIO_6, OUTPUT, 0); // /CS
  pin_write(GPIO_6, true);

  spi_init(GPIO_4, GPIO_5, PORTPIN_NONE, DMA1);

  spi_transaction_t t;
  uint8_t buffer[8];

  t.baud = SPI_BAUD_125K;
  t.bits = (uint8_t) 8;
  t.cke = true;
  t.lsb_first = true;
  t.cs_pin = GPIO_6;
  t.operation = SPI_OPERATION_READ;
  t.write_size = 0x00;
  t.read_size = 0x02;
  t.buffer = &buffer[0];

  spi_queue(&t);

  while(1) {
    // Clear Watchdog.
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Temp: Service SPI. */
    spi_service();

    // Sleep, if there's no time update.
    if (time_service_end()) {
      SLEEP();
    }
  }

  return 0;
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default), base(0x208)) int_default(void) {
}

/* Temp: SPI Interrupt routing. */
void __interrupt(irq(IRQ_SPI2, IRQ_DMA1DCNT), base(0x208)) int_spi2(void) {
  spi_interrupt();
}

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}