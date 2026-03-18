#include <xc.h>
#include <hal/mcu/init.h>
#include <hal/mcu/interrupt.h>

/* Main entry point for control board. */
int main() {
  /* Initialise the MCU memory arbiter. */
  arbiter_init();

  /* Initialise sleep mode. */
  sleep_init();

  /* Initialise vectored interrupt handling. */
  int_init(0x208);

  return 0;
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default),base(0x208)) int_default(void) {
}