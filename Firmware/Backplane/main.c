#include <xc.h>
#include <hal/interrupt.h>
#include <hal/mcu.h>
#include <hal/pin.h>
#include <utils/time.h>
#include "gpio.h"
#include "power.h"
#include <hal/i2c.h>
#include <peripherals/status.h>
#include <peripherals/csp/csp.h>

csp_t csp_front;
csp_t csp_rear;
csp_t csp_bus;

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

  /* Initialise status. */
  status_init(GPIO_STATUS, GPIO_BOOT);

  /* Initialise I2C. */
  i2c_init(GPIO_SCL, GPIO_SDA);

  /* Initialise CSP. */
  csp_init(&csp_front, GPIO_FRONT_UART_RX, GPIO_FRONT_UART_TX, PORTPIN_NONE, 1, CFG_CSP_FULL_DUPLEX);
  csp_init(&csp_rear, GPIO_REAR_UART_RX, GPIO_REAR_UART_TX, PORTPIN_NONE, 3, CFG_CSP_FULL_DUPLEX);
  csp_init(&csp_bus, GPIO_BUS_UART_RX, GPIO_BUS_UART_TX, GPIO_BUS_UART_DE, 2, CFG_CSP_HALF_DUPLEX);

  /* Initialise power manager. */
  power_init();

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

    /* Service CSP. */
    csp_service(&csp_front);
    csp_service(&csp_rear);
    csp_service(&csp_bus);

    /* Service power manager. */
    power_service();

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

/* UART for Bus CSP. */
void __interrupt(irq(IRQ_U2TX), irq(IRQ_U2RX), base(0x208)) int_cs_bus(void) {
  if (PIR8bits.U2TXIF) {
    csp_interrupt_tx(&csp_bus);
  }

  if (PIR8bits.U2RXIF) {
    csp_interrupt_rx(&csp_bus);
  }
}

/* UART for Front CSP. */
void __interrupt(irq(IRQ_U1TX), irq(IRQ_U1RX), base(0x208)) int_csp_front(void) {
  if (PIR4bits.U1TXIF) {
    csp_interrupt_tx(&csp_front);
  }

  if (PIR4bits.U1RXIF) {
    csp_interrupt_rx(&csp_front);
  }
}

/* UART for Rear CSP. */
void __interrupt(irq(IRQ_U3TX), irq(IRQ_U3RX), base(0x208)) int_csp_rear(void) {
  if (PIR9bits.U3TXIF) {
    csp_interrupt_tx(&csp_rear);
  }

  if (PIR9bits.U3RXIF) {
    csp_interrupt_rx(&csp_rear);
  }
}