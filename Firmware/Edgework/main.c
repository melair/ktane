#include "gpio.h"
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

  i2c_transaction_t it;

  uint8_t i2c_buff[2];
  i2c_buff[0] = 0b10000010;
  i2c_buff[1] = 0b00000000;

  it.addr = 0b01110000;
  it.operation = I2C_OPERATION_WRITE;
  it.write_size = 2;
  it.read_size = 0;
  it.buffer = &i2c_buff[0];
  it.callback = NULL;

  i2c_queue(&it);

  i2c_transaction_t it2;

  uint8_t i2c2_buff[2];
  i2c2_buff[0] = 0b10001010;
  i2c2_buff[1] = 0b00010011;

  it2.addr = 0b01110000;
  it2.operation = I2C_OPERATION_WRITE;
  it2.write_size = 2;
  it2.read_size = 0;
  it2.buffer = &i2c2_buff[0];
  it2.callback = NULL;

  i2c_queue(&it2);

  i2c_transaction_t it3;

  uint8_t i2c3_buff[2];
  i2c3_buff[0] = 0b10000100;
  i2c3_buff[1] = 0b00000011;

  it3.addr = 0b01110000;
  it3.operation = I2C_OPERATION_WRITE;
  it3.write_size = 2;
  it3.read_size = 0;
  it3.buffer = &i2c3_buff[0];
  it3.callback = NULL;

  i2c_queue(&it3);

  i2c_transaction_t it4;

  uint8_t i2c4_buff[12];
  i2c4_buff[0] = 0b10000000;
  i2c4_buff[1] = 0b00000000;

  for (uint8_t i = 2; i < 12; i++) {
    i2c4_buff[i] = 0xff;
  }

  it4.addr = 0b01110000;
  it4.operation = I2C_OPERATION_WRITE;
  it4.write_size = 12;
  it4.read_size = 0;
  it4.buffer = &i2c4_buff[0];
  it4.callback = NULL;

  i2c_queue(&it4);

  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    /* Service I2C. */
    i2c_service();

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

/* Timer0 for time. */
void __interrupt(irq(IRQ_TMR0), base(0x208)) int_tmr0(void) {
  time_interrupt();
}
