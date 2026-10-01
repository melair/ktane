#ifndef POWER_STATE_H
#define POWER_STATE_H

/** Observable channel status, independent of power sequencing and management. */
typedef enum {
    POWER_STATE_IDLE = 0,     /**< Output off, no module detected. */
    POWER_STATE_DISABLED = 1, /**< Output off, module detected; not an inhibit. */
    POWER_STATE_ACTIVE = 2,   /**< eFuse output commanded on. */
    POWER_STATE_TRIPPED = 3,  /**< Fault latched, output off. */
} Power_State;

#endif // POWER_STATE_H
