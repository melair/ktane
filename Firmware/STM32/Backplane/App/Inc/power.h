#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POWER_CHANNEL_FRONT = 0,
    POWER_CHANNEL_REAR,
    POWER_CHANNEL_COUNT,
} Power_ChannelId;

bool Power_Init(void);

void Power_Service(void);

/**
 * Check whether a module is connected to a power channel.
 *
 * @param channel Channel to check.
 * @return true if a module is detected; false otherwise or if the channel is
 *         invalid.
 */
bool Power_IsModuleDetected(Power_ChannelId channel);

/**
 * Check whether a power channel output is enabled.
 *
 * @param channel Channel to check.
 * @return true if the channel's eFuse output is enabled; false otherwise or
 *         if the channel is invalid.
 */
bool Power_IsActive(Power_ChannelId channel);

/**
 * Check whether a power channel is in the tripped state.
 *
 * @param channel Channel to check.
 * @return true if the channel is tripped; false otherwise or if the channel
 *         is invalid.
 */
bool Power_IsTripped(Power_ChannelId channel);

/**
 * Get the measured current draw for an active power channel.
 *
 * @param channel Channel to read.
 * @return Averaged current draw in milliamps, or 0 for an invalid or inactive
 *         channel.
 */
uint16_t Power_GetCurrent(Power_ChannelId channel);

/**
 * Get the applied current limit for a power channel.
 *
 * @param channel Channel to read.
 * @return Current limit in 0.1 A units, or 0 for an invalid channel.
 */
uint8_t Power_GetCurrentLimit(Power_ChannelId channel);

/**
 * Enable or disable a power channel.
 *
 * Enabling is available only at non-chassis locations while the channel is
 * idle and a module is detected. The default current limit is written before
 * output is enabled. Disabling cancels a pending activation or transitions an
 * active or tripped channel to shutdown.
 *
 * @param channel Channel to control.
 * @param enabled true to request power on; false to request power off.
 * @return true if the request was accepted; false for an invalid channel or
 *         an enable request that cannot start from the current state.
 */
bool Power_SetEnabled(Power_ChannelId channel, bool enabled);

/**
 * Request a new current limit for an active power channel.
 *
 * @param channel Channel to update.
 * @param current_limit_deciamps Current limit in 0.1 A units, from 0 to 20.
 * @return true if the request was accepted; false if it was invalid or the
 *         channel was inactive.
 */
bool Power_SetCurrentLimit(Power_ChannelId channel, uint8_t current_limit_deciamps);

#ifdef __cplusplus
}
#endif

#endif //POWER_H
