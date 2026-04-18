#include "gpio.h"
#include "hal/pin.h"
#include <hal/argb.h>
#include <hal/i2c.h>
#include <hal/interrupt.h>
#include <hal/mcu.h>
#include <hal/spi.h>
#include <peripherals/csp/csp.h>
#include <peripherals/status.h>
#include <utils/time.h>
#include <xc.h>

csp_t csp_backplane;

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

  /* Initialise status. */
  status_init(GPIO_STATUS, GPIO_BOOT);

  /* Initialise I2C. */
  i2c_init(GPIO_SCL, GPIO_SDA);

  /* Initialise SPI. */
  spi_init(GPIO_SPI_COPI, GPIO_SPI_CLK, GPIO_SPI_CIPO);

  /* Initialise CSP. */
  csp_init(&csp_backplane, GPIO_UART_RX, GPIO_UART_TX, PORTPIN_NONE, 2,
           CFG_CSP_FULL_DUPLEX);

  /* Intialise ARGB */
  argb_init(GPIO_ARGB, false);
  argb_set(0x00, 0x00, 0x00, 0x00);
  argb_update();

  pin_t LATCH = GPIO_B0;
  pin_t BLANK = GPIO_B1;

  pin_config(LATCH, OUTPUT, 0);
  pin_config(BLANK, OUTPUT, 0);
  pin_write(LATCH, true);
  pin_write(BLANK, false);

  spi_transaction_t t;

  uint8_t buffer[4];

  t.baud = SPI_BAUD_125K;
  t.cs_pin = LATCH;
  t.buffer = &buffer[0];
  t.write_size = 4;
  t.write_repeats = 0;
  t.operation = SPI_OPERATION_WRITE;
  t.callback = NULL;
  t.cs_wait_ms = 0;
  t.cke = 0;
  t.ckp = 1;
  t.lsb_first = 0;
  t.bits = 8;

  buffer[0] = 0b00110100;
  buffer[1] = 0b11011100;
  buffer[2] = 0b00110100;
  buffer[3] = 0b11011100;

  spi_queue(&t);

  argb_led_t leds[ARGB_DEFAULT_BUFFER_SIZE + 47];
  argb_set_buffer(&leds[0], ARGB_DEFAULT_BUFFER_SIZE + 47);

  argb_set(0, 0xff, 0x00, 0x00);
  argb_set(1, 0xff, 0x00, 0x00);
  argb_set(2, 0xff, 0x00, 0x00);
  argb_set(3, 0xff, 0x00, 0x00);
  argb_set(4, 0xff, 0x00, 0x00);
  argb_set(5, 0xff, 0x00, 0x00);
  argb_set(7, 0xff, 0x00, 0x00);
  argb_set(8, 0xff, 0x00, 0x00);
  argb_set(9, 0xff, 0x00, 0x00);
  argb_set(10, 0xff, 0x00, 0x00);
  argb_set(11, 0xff, 0x00, 0x00);

  argb_set(14, 0xff, 0x00, 0x00);
  argb_set(15, 0xff, 0x00, 0x00);
  argb_set(16, 0xff, 0x00, 0x00);
  argb_set(17, 0xff, 0x00, 0x00);
  argb_set(20, 0xff, 0x00, 0x00);
  argb_set(21, 0xff, 0x00, 0x00);
  argb_set(22, 0xff, 0x00, 0x00);
  argb_set(24, 0xff, 0x00, 0x00);
  argb_set(25, 0xff, 0x00, 0x00);
  argb_set(29, 0xff, 0x00, 0x00);
  argb_set(30, 0xff, 0x00, 0x00);
  argb_set(35, 0xff, 0x00, 0x00);
  argb_set(36, 0xff, 0x00, 0x00);

  argb_set(37, 0xff, 0x00, 0x00);
  argb_set(38, 0xff, 0x00, 0x00);
  argb_set(40, 0xff, 0x00, 0x00);
  argb_set(41, 0xff, 0x00, 0x00);
  argb_set(42, 0xff, 0x00, 0x00);
  argb_set(46, 0xff, 0x00, 0x00);
  argb_set(47, 0xff, 0x00, 0x00);
  argb_update();

  /* Main loop. */
  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Service status. */
    status_service();

    /* Service I2C. */
    i2c_service();

    /* Service SPI. */
    spi_service();

    /* Service CSP. */
    csp_service(&csp_backplane);

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
void __interrupt(irq(default), base(0x208)) int_default(void) {}

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}

/* I2C and it's DMA. */
void __interrupt(irq(IRQ_I2C1, IRQ_I2C1E, IRQ_DMA2DCNT), base(0x208))
    int_i2c(void) {
  i2c_interrupt();
}

/* SPI and it's DMA. */
void __interrupt(irq(IRQ_SPI2, IRQ_DMA1DCNT), base(0x208)) int_spi(void) {
  spi_interrupt();
}

/* ARGB's SPI. */
void __interrupt(irq(IRQ_SPI1), base(0x208)) int_argb(void) {
  argb_interrupt();
}

/* UART for CSP. */
void __interrupt(irq(IRQ_U2TX), irq(IRQ_U2RX), base(0x208)) int_csp(void) {
  if (PIR8bits.U2TXIF) {
    csp_interrupt_tx(&csp_backplane);
  }

  if (PIR8bits.U2RXIF) {
    csp_interrupt_rx(&csp_backplane);
  }
}