/**
 * Main file for KTANE, containing main loop and MCU initialisation functions.
 *
 * Firmware is designed to be non blocking, there should be no loops to wait
 * for either time, or a register bit to clear/set. The PICs peripherals all
 * support interrupt driven operation.
 *
 * As a result, code tends to be split between an interrupt for handling things
 * that can be processed very quickly (such as incrementing a value, moving
 * along a buffer), and operations which can take a longer time, or can be
 * called by other features happen in a service loop. Service loops should
 * always endeavour to only perform actions if something has changed.
 */
#include <xc.h>
#include <stdbool.h>
#include "argb.h"
#include "buzzer.h"
#include "edgework.h"
#include "game.h"
#include "interrupt.h"
#include "main.h"
#include "mode.h"
#include "module.h"
#include "eeprom.h"
#include "serial.h"
#include "status.h"
#include "tick.h"
#include "rng.h"
#include "hal/spi.h"
#include "opts.h"
#include "fw_server.h"
#include "../common/can.h"
#include "../common/mcu.h"
#include "../common/device.h"
#include "../common/fw.h"
#include "../common/fw_updater.h"

/* Current firmware version, big endian. */
asm("PSECT applicationversion");
asm("dw 0x0058");

/**
 * Main function, initialise and main loop.
 */
void main(void) {
    /* Drive everything unused to low. */
    safe_unused_pins();

    /* Reconfigure system arbiter. */
    arbiter_initialise();

    /* Configure sleep behaviour, put CPU in IDLE. */
    CPUDOZEbits.IDLEN = 1;

    /* Initialise interrupts. */
    int_initialise();

    /* Load versions from PFM. */
    fw_initialise();

    /* Unlock PPS during initialisation. */
    pps_unlock();

    /* Initialise tick. */
    tick_initialise();

    /* Initialise EEPROM, including data migrations. */
    eeprom_initialise();

    /* Initialise RNG. */
    rng_initialise();

    /* Initialise serial number. */
    serial_initialise();

    /* Initialise CAN bus. */
    can_initialise();

    /* Initialise SPI. */
    spi_initialise();

    /* Initialise ARGB. */
    argb_initialise();

    /* Initialise Buzzer. */
    buzzer_initialise();

    /* Initialise optional modules. */
    opts_initialise();

    /* Initialise the mode. */
    mode_initialise();

    /* Initialise modules. */
    module_initialise();

    /* Initialise game state. */
    game_initialise();

    /* Beep on start. */
    buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 100);

    /* Set status led to ready. */
    status_set(STATUS_READY);

    /* Main loop. */
    while (true) {
        /* Clear watchdog. */
        CLRWDT();

        /* Service the tick. */
        tick_service();

        /* Service CAN buffers. */
        can_service();

        /* Update module if CAN details are dirty. */
        if (can_dirty()) {
            module_set_self_can_id(can_get_id());
        }

        /* Service SPI. */
        spi_service();

        /* Service optional modules. */
        opts_service();

        /* Service module subsystem. */
        module_service();

        /* Service the game state. */
        game_service();

        /* Update ARGB string if needed. */
        argb_service();

        /* Service buzzer driver. */
        buzzer_service();

        /* Service status LED. */
        status_service();

        /* Service edgework. */
        edgework_service();

        /* Service the mode. */
        mode_service();

        /* Service RNG. */
        rng_service();

        /* Put MCU into IDLE mode, no CPU instructions but peripherals continue.
         *
         * Full SLEEP would be ideal, but is incompatible with the CAN peripheral
         * as SLEEP disables the memory controller. CAN can wake upon start of
         * message but the packet would likely be lost. This behaviour would be
         * suitable for a device that comes on once every few seconds, but for
         * something that needs to wake up at least every 200ms, it wont work.
         *
         * CPU will wake from the tick timer. This doesn't save much power, but
         * it prevents functions spinning that don't need to operate faster than
         * the tick refresh. */
        SLEEP();
    }
}
