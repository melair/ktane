#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {



#endif

typedef uint8_t EdgeworkMode;

#define MODE_SERIAL      0x00U
#define MODE_INDICATOR   0x01U
#define MODE_BATTERY_ONE 0x02U
#define MODE_BATTERY_TWO 0x03U
#define MODE_PORTS       0x04U
#define MODE_2FA         0x05U
#define MODE_CONTROLLER  0xfeU
#define MODE_UNKNOWN     0xffU

bool Mode_Init(void);

EdgeworkMode Mode_Get(void);

bool Mode_Set(EdgeworkMode mode);

#ifdef __cplusplus
}
#endif

#endif // MODE_H
