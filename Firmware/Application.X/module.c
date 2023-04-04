#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "protocol_module.h"
#include "module.h"
#include "tick.h"
#include "mode.h"
#include "buzzer.h"
#include "argb.h"
#include "status.h"
#include "game.h"
#include "serial.h"
#include "../common/fw.h"
#include "../common/segments.h"
#include "../common/can.h"

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
    modules[0].domain = can_get_domain();
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
 * Set this modules CAN domain in the module record.
 *
 * @param id new domain
 */
void module_set_self_domain(uint8_t domain) {
    modules[0].domain = domain;
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
        protocol_module_announcement_send();
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
    
    if (game.state == GAME_IDLE && !this_module->enabled) {
        this_module->enabled = true;
    }

    if (tick_20hz) {
        error_state = module_determine_error_state();
        if (error_state_prev != error_state) {
            status_error(error_state);
            error_state_prev = error_state;
        }
    }
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
void module_seen(uint8_t id, uint8_t mode, uint16_t app_fw, uint32_t serial, uint8_t domain, bool debug, uint16_t boot_fw, uint16_t flasher_fw) {
    uint8_t idx = module_find_or_create(id);

    if (idx == 0xff) {
        return;
    }

    if (modules[idx].flags.LOST) {
        module_error_raise(MODULE_ERROR_CAN_LOST_BASE | id, false);
    }

    modules[idx].mode = mode;
    modules[idx].firmware.bootloader = boot_fw;
    modules[idx].firmware.application = app_fw;
    modules[idx].firmware.flasher = flasher_fw;
    modules[idx].serial = serial;
    modules[idx].last_seen = tick_value;
    modules[idx].flags.LOST = 0;
    modules[idx].flags.DEBUG = (debug ? 1 : 0);
    modules[idx].game.puzzle = (mode >= MODE_PUZZLE_BASE);
    modules[idx].game.needy = (mode >= MODE_NEEDY_KEYS);
    modules[idx].domain = domain;
}

/**
 * Raise an error for this module, store in our module database and send packet
 * on CAN bus to let other modules be aware of our error (primarily for puzzle
 * modules to report to controller).
 *
 * @param code error code to raise
 */
void module_error_raise(uint16_t code, bool active) {
    protocol_module_error_send(code, active);
    module_error_record(can_get_id(), code, active);
}

/**
 * Update the module database with an error that has been received.
 *
 * @param id CAN id
 * @param code error code received
 */
void module_error_record(uint8_t id, uint16_t code, bool active) {
    uint8_t idx = module_find_or_create(id);

    if (idx == 0xff) {
        return;
    }

    for (uint8_t i = 0; i < ERROR_COUNT; i++) {
        if (modules[idx].errors[i].code == code) {
            if (active) {
                modules[idx].errors[i].count++;
            }
            modules[idx].errors[i].active = active;
            return;
        }

        if (modules[idx].errors[i].code == MODULE_ERROR_NONE) {
            modules[idx].errors[i].code = code;
            modules[idx].errors[i].count = 1;
            modules[idx].errors[i].active = active;
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