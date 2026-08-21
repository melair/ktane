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
 * Request a new current limit for an active power channel.
 *
 * @param channel Channel to update.
 * @param current_deciamps Current limit in 0.1 A units, from 0 to 20.
 * @return true if the request was accepted; false if it was invalid or the
 *         channel was inactive.
 */
bool Power_SetCurrent(Power_ChannelId channel, uint8_t current_deciamps);

#ifdef __cplusplus
}
#endif

#endif //POWER_H
