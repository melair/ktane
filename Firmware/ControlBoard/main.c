#include "gpio.h"
#include "hal/pin.h"
#include <hal/argb.h>
#include <hal/i2c.h>
#include <hal/interrupt.h>
#include <hal/mcu.h>
#include <hal/spi.h>
#include <hal/can.h>
#include <language_support.h>
#include <peripherals/csp/csp.h>
#include <peripherals/status.h>
#include <utils/time.h>
#include <xc.h>

csp_t csp_backplane = {.callback = NULL};

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

  /* Initialise CAN. */
  can_init(GPIO_CAN_TX, GPIO_CAN_RX, GPIO_CAN_ACT, NULL);

  /* Initialise CSP. */
  csp_init(&csp_backplane, GPIO_UART_RX, GPIO_UART_TX, PORTPIN_NONE, 2,
           CFG_CSP_FULL_DUPLEX);

  /* Intialise ARGB */
  argb_init(GPIO_ARGB, false);
  argb_set(0x00, 0x00, 0x00, 0x00);
  argb_update();

  /* Main loop. */
  while (true) {
    /* Clear Watchdog. */
    CLRWDT();

    /* Start the timer processing. */
    time_service_start();

    if (tick_2hz) {
      uint8_t d[4] = {0x01, 0x02, 0x03, 0x04};
      can_tx(0xaa, 4, &d[0]);
    }


    /* Service status. */
    status_service();

    /* Service I2C. */
    i2c_service();

    /* Service SPI. */
    spi_service();

    /* Service CAN. */
    can_service();

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

/* CAN. */
void __interrupt(irq(IRQ_CAN), base(0x208)) int_can(void) {
  can_interrupt();
}