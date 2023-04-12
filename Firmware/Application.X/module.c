#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "module.h"
#include "tick.h"
#include "mode.h"
#include "buzzer.h"
#include "argb.h"
#include "status.h"
#include "game.h"
#include "serial.h"
#include "status.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"
#include "../common/fw.h"
#include "../common/fw_updater.h"
#include "../common/segments.h"
#include "../common/can.h"
#include "../common/protocol.h"
#include "../common/packet.h"

/* How frequent should lost modules be checked for, in 1ms units. */
#define LOST_CHECK_PERIOD   100
/* How frequent should module announcements be, in 1ms units. */
#define ANNOUNCE_PERIOD     200
/* How long should it be before we declare a module missing, in 1ms units. */
#define LOST_PERIOD         400

/* The clock tick that this module should check for lost nodes. */
uint32_t next_lost_check;
/* The clock tick that this module should next announce. */
uint32_t next_announce;

/* Modules in network. */
module_t modules[MODULE_COUNT];

/* Error state representing error tables. */
uint8_t error_state = ERROR_NONE;
/* Previously reported error state. */
uint8_t error_state_prev = ERROR_NONE;

/* Local function prototypes. */
uint8_t module_find(uint8_t id);
uint8_t module_find_or_create(uint8_t id);
uint8_t module_determine_error_state(void);
void module_announce(void);
void module_errors_clear(uint8_t id);

/**
 * Initialise the module database, store self and set up next announcement time.
 */
void module_initialise(void) {
    /* Init module structure. */
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        modules[i].flags.INUSE = 0;

        for (uint8_t j = 0; j < ERROR_COUNT; j++) {
            modules[i].errors[j].code = MODULE_ERROR_NONE;
            modules[i].errors[j].count = 0;
            modules[i].errors[j].active = 0;
        }

        modules[i].game.enabled = false;
        modules[i].game.ready = false;
    }

    /* Store this module in slot 0. */
    modules[0].flags.INUSE = 1;
    modules[0].flags.LOST = 0;
#ifdef __DEBUG
    modules[0].flags.DEBUG = 1;
#endif
    modules[0].id = can_get_id();
    modules[0].mode = mode_get();
    modules[0].serial = serial_get();
    modules[0].firmware.bootloader = fw_version(BOOTLOADER);
    modules[0].firmware.application = fw_version(APPLICATION);
    modules[0].firmware.flasher = fw_version(FLASHER);

    /* Set next module announce time, offsetting with modulus of CAN ID to
     * attempt to avoid collisions. */
    next_announce = tick_value + ANNOUNCE_PERIOD + (can_get_id() & 0x0f);

    /* Set next lost check. */
    next_lost_check = LOST_CHECK_PERIOD;
}

/**
 * Set this modules CAN id in the module record.
 *
 * @param id new can ID
 */
void module_set_self_can_id(uint8_t id) {
    modules[0].id = id;
    modules[0].game.id = id;
}

/**
 * Clear all errors off the specified module.
 *
 * @param id CAN id to clear errors of
 */
void module_errors_clear(uint8_t id) {
    uint8_t i = module_find(id);

    if (i == 0xff) {
        return;
    }

    for (uint8_t j = 0; j < ERROR_COUNT; j++) {
        modules[i].errors[j].code = MODULE_ERROR_NONE;
        modules[i].errors[j].count = 0;
        modules[i].errors[j].active = 0;
    }
}

/**
 * Service modules needs, including sending announcements and checking for other
 * lost modules.
 */
void module_service(void) {
    /* If module is still initing, then don't do anything. */
    if (game.state == GAME_INIT) {
        return;
    }

    /* Announce self. */
    if (tick_value >= next_announce) {
        /* Set next announce time. */
        next_announce = tick_value + ANNOUNCE_PERIOD;

        /* Send module announcement. */
        module_announce();
    }

    /* Check for lost nodes frequently. */
    if (tick_value >= next_lost_check) {
        next_lost_check = tick_value + LOST_CHECK_PERIOD;

        /* Check for lost nodes. */
        for (uint8_t i = 1; i < MODULE_COUNT && modules[i].flags.INUSE; i++) {
            uint32_t expiryTime = modules[i].last_seen + LOST_PERIOD;
            if (modules[i].flags.INUSE && !modules[i].flags.LOST && (expiryTime < tick_value)) {
                modules[i].flags.LOST = 1;
                module_error_raise(MODULE_ERROR_CAN_LOST_BASE | modules[i].id, true);
            }
        }
    }

    /* If we are in OVER state, then set the module to be unready. */
    if (game.state == GAME_OVER && this_module->ready) {
        this_module->ready = false;
    }

    if (tick_20hz) {
        error_state = module_determine_error_state();
        if (error_state_prev != error_state) {
            status_error(error_state);
            error_state_prev = error_state;
        }
    }
}

bool announce_reset = true;

void module_announce(void) {
    packet_outgoing.module.announcement.mode = mode_get();
    packet_outgoing.module.announcement.application_version = fw_version(APPLICATION);
    packet_outgoing.module.announcement.flags.reset = announce_reset;
#ifdef __DEBUG
    packet_outgoing.module.announcement.flags.debug = true;
#endif
    packet_outgoing.module.announcement.serial = serial_get();
    packet_outgoing.module.announcement.bootloader_version = fw_version(BOOTLOADER);
    packet_outgoing.module.announcement.flasher_version = fw_version(FLASHER);
    packet_send(PREFIX_MODULE, OPCODE_MODULE_ANNOUNCEMENT, SIZE_MODULE_ANNOUNCEMENT, &packet_outgoing);

    announce_reset = false;
}

/**
 * Determine the overall error state of the module. If the module is a controller
 * this will represent the whole network, if anything else only its own state.
 *
 * @return the error state
 */
uint8_t module_determine_error_state(void) {
    for (uint8_t i = 0; i < ERROR_COUNT; i++) {
        if (modules[0].errors[i].code != MODULE_ERROR_NONE && modules[0].errors[i].active == 1) {
            return ERROR_LOCAL;
        }
    }

    uint8_t ret = ERROR_NONE;

    if (mode_get() == MODE_CONTROLLER) {
        for (uint8_t i = 1; i < MODULE_COUNT && modules[i].flags.INUSE; i++) {
            for (uint8_t j = 0; j < ERROR_COUNT; j++) {
                if (modules[i].errors[j].code != MODULE_ERROR_NONE) {
                    if (modules[i].errors[j].active) {
                        return ERROR_REMOTE_ACTIVE;
                    } else {
                        ret = ERROR_REMOTE_INACTIVE;
                    }
                }
            }
        }
    }

    return ret;
}

/**
 * Find a module in the database.
 *
 * @param id CAN id
 * @return the modules index, or 0xff if not found
 */
uint8_t module_find(uint8_t id) {
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        if (modules[i].flags.INUSE) {
            if (modules[i].id == id) {
                return i;
            }
        } else {
            return 0xff;
        }
    }

    return 0xff;
}

/**
 * Find or create a module in the database.
 *
 * @param id CAN id
 * @return the modules index, or 0xff if none could be created
 */
uint8_t module_find_or_create(uint8_t id) {
    uint8_t idx = module_find(id);

    if (idx != 0xff) {
        return idx;
    }

    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        if (!modules[i].flags.INUSE) {
            modules[i].flags.INUSE = 1;
            modules[i].id = id;
            modules[i].game.id = id;
            return i;
        }
    }

    return 0xff;
}

/**
 * Update the module database to mark a module as seen, including the mode that
 * the module has advertised.
 *
 * @param id CAN id
 * @param mode modules mode
 * @param firmware firmware version
 * @param serial serial number of the module
 */
void module_receive_announce(uint8_t id, packet_t *p) {
    uint8_t idx = module_find_or_create(id);

    if (idx == 0xff) {
        return;
    }

    if (modules[idx].flags.LOST) {
        module_error_raise(MODULE_ERROR_CAN_LOST_BASE | id, false);
    }

    modules[idx].mode = p->module.announcement.mode;
    modules[idx].firmware.bootloader = p->module.announcement.bootloader_version;
    modules[idx].firmware.application = p->module.announcement.application_version;
    modules[idx].firmware.flasher = p->module.announcement.flasher_version;
    modules[idx].serial = p->module.announcement.serial;
    modules[idx].last_seen = tick_value;
    modules[idx].flags.LOST = 0;
    modules[idx].flags.DEBUG = p->module.announcement.flags.debug;
    modules[idx].game.puzzle = (modules[idx].mode >= MODE_PUZZLE_BASE);
    modules[idx].game.needy = (modules[idx].mode >= MODE_NEEDY_KEYS);

    if (p->module.announcement.flags.reset) {
        module_errors_clear(id);
    }

    if (!p->module.announcement.flags.debug && modules[idx].mode == MODE_CONTROLLER) {
        /* Update the firmwares in the following order: BOOTLOADER, FLASHER, APPLICATION. */
        if (modules[idx].firmware.bootloader > fw_version(BOOTLOADER)) {
            fw_updater_start(BOOTLOADER, modules[idx].firmware.bootloader);
        } else if (modules[idx].firmware.flasher > fw_version(FLASHER)) {
            fw_updater_start(FLASHER, modules[idx].firmware.flasher);
        } else if (modules[idx].firmware.application > fw_version(APPLICATION)) {
            /* Application can not update its self, bump to flasher. */
            nvm_eeprom_write(EEPROM_LOC_BOOTLOADER_TARGET, 0x00);
            nvm_eeprom_write(EEPROM_LOC_FLASHER_SEGMENT, APPLICATION);
            nvm_eeprom_write(EEPROM_LOC_FLASHER_VERSION_HIGHER, (modules[idx].firmware.application >> 8) & 0xff);
            nvm_eeprom_write(EEPROM_LOC_FLASHER_VERSION_LOWER, modules[idx].firmware.application & 0xff);
            RESET();
        }
    }
}

/**
 * Raise an error for this module, store in our module database and send packet
 * on CAN bus to let other modules be aware of our error (primarily for puzzle
 * modules to report to controller).
 *
 * @param code error code to raise
 */
void module_error_raise(uint16_t code, bool active) {
    packet_outgoing.module.error_announcement.error_code = code;
    packet_outgoing.module.error_announcement.flags.active = active;
    packet_send(PREFIX_MODULE, OPCODE_MODULE_ERROR, SIZE_MODULE_ERROR, &packet_outgoing);

    module_receive_error(can_get_id(), &packet_outgoing);
}

/**
 * Update the module database with an error that has been received.
 *
 * @param id CAN id
 * @param code error code received
 */
void module_receive_error(uint8_t id, packet_t *p) {
    uint8_t idx = module_find_or_create(id);

    if (idx == 0xff) {
        return;
    }

    for (uint8_t i = 0; i < ERROR_COUNT; i++) {
        if (modules[idx].errors[i].code == p->module.error_announcement.error_code) {
            if (p->module.error_announcement.flags.active) {
                modules[idx].errors[i].count++;
            }
            modules[idx].errors[i].active = p->module.error_announcement.flags.active;
            return;
        }

        if (modules[idx].errors[i].code == MODULE_ERROR_NONE) {
            modules[idx].errors[i].code = p->module.error_announcement.error_code;
            modules[idx].errors[i].count = 1;
            modules[idx].errors[i].active = p->module.error_announcement.flags.active;
            return;
        }
    }

    uint8_t ovr = ERROR_COUNT - 1;

    if (modules[idx].errors[ovr].code == MODULE_ERROR_TOO_MANY_ERRORS) {
        modules[idx].errors[ovr].count++;
    } else {
        modules[idx].errors[ovr].code = MODULE_ERROR_TOO_MANY_ERRORS;
        modules[idx].errors[ovr].count = 1;
    }

    modules[idx].errors[ovr].active = true;
}

/**
 * Return the game data for a specific module index. Consumers should iterate
 * from 0 up to MODULE_COUNT, if a NULL is returned that is the end of the list.
 *
 * @param idx module index in data store to fetch
 * @return game data for module index, NULL at end of list
 */
module_game_t *module_get_game(uint8_t idx) {
    if (idx >= MODULE_COUNT) {
        return NULL;
    }

    if (!modules[idx].flags.INUSE) {
        return NULL;
    }

    return &modules[idx].game;
}

/**
 * Return the game data for a specific module by CAN id.
 *
 * @param id CAN id to get data for
 * @return game data for module index, NULL if not present
 */
module_game_t *module_get_game_by_id(uint8_t id) {
    uint8_t idx = module_find(id);

    if (idx == 0xff) {
        return NULL;
    }

    return &modules[idx].game;
}

/**
 * Return the module for the specified module index, if a NULL is return that is
 * the end of the list.
 *
 * @param idx module index in data store to fetch
 * @return module data for module index, NULL at end of list
 */
module_t *module_get(uint8_t idx) {
    if (idx >= MODULE_COUNT) {
        return NULL;
    }

    if (!modules[idx].flags.INUSE) {
        return NULL;
    }

    return &modules[idx];
}

/**
 * Return the errors on a module by module index, is a NULL if the module does
 * not exist.
 *
 * @param idx module index in data store to fetch
 * @return module errors for module index, NULL at end of list
 */
module_error_t *module_get_errors(uint8_t idx, uint8_t err) {
    if (idx >= MODULE_COUNT) {
        return NULL;
    }

    if (!modules[idx].flags.INUSE) {
        return NULL;
    }

    if (err >= ERROR_COUNT) {
        return NULL;
    }

    return &modules[idx].errors[err];
}

void module_send_reset(void) {
    packet_send(PREFIX_MODULE, OPCODE_MODULE_RESET, SIZE_MODULE_RESET, &packet_outgoing);

    /* Wait until the CAN TX buffer is empty, i.e. the above packet has gone. */
    while (!C1TXQSTALbits.TXQEIF);
    /* Reset the MCU. */
    RESET();
}

void module_receive_reset(uint8_t id, packet_t *p) {
    RESET();
}

void module_send_identify(uint8_t id) {
    packet_outgoing.module.identify.can_id = id;
    packet_send(PREFIX_MODULE, OPCODE_MODULE_IDENTIFY, SIZE_MODULE_IDENTIFY, &packet_outgoing);
    module_receive_identify(0, &packet_outgoing);
}

void module_receive_identify(uint8_t id, packet_t *p) {
    status_identify(p->module.identify.can_id == can_get_id());
}

void module_send_mode_set(uint8_t id, uint8_t mode) {
    packet_outgoing.module.set_mode.can_id = id;
    packet_outgoing.module.set_mode.mode = mode;
    packet_send(PREFIX_MODULE, OPCODE_MODULE_MODE_SET, SIZE_MODULE_MODE_SET, &packet_outgoing);
}

void module_receive_mode_set(uint8_t id, packet_t *p) {
    /* If targeted at this module. */
    if (p->module.set_mode.can_id == can_get_id()) {
        /* Change module mode in EEPROM. */
        nvm_eeprom_write(EEPROM_LOC_MODE_CONFIGURATION, p->module.set_mode.mode);

        /* Reset the MCU. */
        RESET();
    }
}

void module_send_special_function(uint8_t id, uint8_t special_fn) {
    packet_outgoing.module.special_function.can_id = id;
    packet_outgoing.module.special_function.special_function = special_fn;
    packet_send(PREFIX_MODULE, OPCODE_MODULE_SPECIAL_FUNCTION, SIZE_MODULE_SPECIAL_FUNCTION, &packet_outgoing);
}

void module_receive_special_function(uint8_t id, packet_t *p) {
    /* If targeted at this module. */
    if (p->module.special_function.can_id == can_get_id()) {
        mode_call_special_function(p->module.special_function.special_function);
    }
}