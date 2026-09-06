#ifndef BACKPLANE_H
#define BACKPLANE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t BackplaneLocation;

#define BACKPLANE_LOCATION_0       0x00U
#define BACKPLANE_LOCATION_1       0x01U
#define BACKPLANE_LOCATION_2       0x02U
#define BACKPLANE_LOCATION_3       0x03U
#define BACKPLANE_LOCATION_4       0x04U
#define BACKPLANE_LOCATION_5       0x05U
#define BACKPLANE_LOCATION_CHASSIS 0xfeU
#define BACKPLANE_LOCATION_UNKNOWN 0xffU

bool Backplane_Init(void);

BackplaneLocation Backplane_GetLocation(void);

bool Backplane_SetLocation(BackplaneLocation location);

#ifdef __cplusplus
}
#endif

#endif // BACKPLANE_H
