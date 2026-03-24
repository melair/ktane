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
#include <peripherals/epaper/epaper.h>

epaper_t epaper;

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
  spi_init(GPIO_4, GPIO_5, PORTPIN_NONE, DMA1);

  /* Temp: Initialise ePaper. */
  pin_config(GPIO_6, OUTPUT, 0); // /CS
  pin_write(GPIO_6, true);
  pin_config(GPIO_0, OUTPUT, 0); // PWR
  pin_config(GPIO_7, OUTPUT, 0); // D/~C
  pin_config(GPIO_8, OUTPUT, 0); // RESET
  pin_config(GPIO_9, INPUT, 0); // BUSY
  pin_write(GPIO_8, true);

  epaper.cs = GPIO_6;
  epaper.pwr = GPIO_0;
  epaper.dc = GPIO_7;
  epaper.reset = GPIO_8;
  epaper.busy = GPIO_9;
  epaper.type = EPAPER_TYPE_SSD1680;
  epaper.width = 296;
  epaper.height = 128;
  epaper.spi_transaction.cs_pin = epaper.cs;
  epaper.spi_transaction.cs_bounce = true;
  epaper.spi_transaction.cs_wait_ms = 0;
  epaper.spi_transaction.baud = SPI_BAUD_125K;
  epaper.spi_transaction.bits = 8;
  epaper.spi_transaction.cke = 1;
  epaper.spi_transaction.lsb_first = 0;
  epaper.spi_transaction.operation = SPI_OPERATION_WRITE;
  epaper.spi_transaction.write_repeats = 0;
  epaper.spi_transaction.write_size = 0;
  epaper.spi_transaction.read_size = 0;
  epaper.spi_transaction.callback_data = &epaper;
  epaper_init(&epaper);

  epaper_command_t a;
  epaper_queue(&epaper, &a);
  epaper_refresh(&epaper, false);

  while(1) {
    // Clear Watchdog.
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Temp: Service SPI. */
    spi_service();

    /* Temp: Service ePaper */
    epaper_service(&epaper);

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