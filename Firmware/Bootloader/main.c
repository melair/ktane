#include <hal/nvm/nvm.h>
#include <stdint.h>
#include <xc.h>

#define APPLICATION_COUNT 4
#define APPLICATION_DEFAULT 0x00

#define NO_APPLICATION 0xFFFFFF

const uint24_t APPLICATIONS[APPLICATION_COUNT] = {
    0x000200, NO_APPLICATION, NO_APPLICATION, NO_APPLICATION};

uint24_t select_application(void);

/* Main bootloader function. */
int main() {
  /* Resolve boot address. */
  uint24_t boot_address = select_application();

  /* Reset stack pointer. */
  STKPTR = 0x00;

  /* Reset interrupt return. */
  BSR = 0x00;

  /* Copy address into the PC latch addresses, and then finally PCL which jumps
   * us. */
  PCLATU = (uint8_t)(boot_address >> 16);
  PCLATH = (uint8_t)(boot_address >> 8);
  PCL = (uint8_t)(boot_address >> 0);

  /* Never reached. */
  return 0;
}

/* Find which application we're loading, and return the jump address. */
uint24_t select_application(void) {
  uint8_t selected = APPLICATION_DEFAULT;

#ifdef BOOT_SELECTOR
  /* If the boot button is held down at power on, boot the last application on
   * the MCU. */
  if (BOOT_SELECTOR_BUTTON_PORTbits == 0) {
    for (uint8_t i = APPLICATION_COUNT; i > 0; i--) {
      if (APPLICATIONS[i - 1] != NO_APPLICATION) {
        selected = i - 1;
        break;
      }
    }
  }
#endif

  /* If the boot selector has not been used, read from EEPROM. */
  if (selected == APPLICATION_DEFAULT) {
    uint8_t app = nvm_eeprom_read(0x000);

    if (app != 0xFF) {
      selected = app;
      nvm_eeprom_write(0x000, 0xFF);
    }
  }

  /* If nothing else, boot the default application. */
  return APPLICATIONS[selected];
}
