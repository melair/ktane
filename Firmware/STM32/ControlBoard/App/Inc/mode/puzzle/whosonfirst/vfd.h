#ifndef WHOSONFIRST_VFD_H
#define WHOSONFIRST_VFD_H

#include <stdbool.h>
#include <stdint.h>
#include "fsm/fsm.h"
#include "spi/spi.h"
#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VFD_CHARACTER_COUNT 8U

typedef struct {
    GPIO_PinDef enable;
    GPIO_PinDef reset;
    GPIO_PinDef cs;
} VFD_Config;

typedef struct {
    VFD_Config config;
    FSM fsm;
    SPI_Transaction transaction;
    uint8_t pending_characters[VFD_CHARACTER_COUNT];
    uint8_t write_buffer[VFD_CHARACTER_COUNT + 1U];
    bool render_pending;
    bool initialized;
} VFD;

/* Configures the pins with the voltage booster disabled, then starts the
 * asynchronous power-on and controller-initialization sequence. */
bool VFD_Init(VFD *vfd, const VFD_Config *config);

/* Advance the power, initialization, and render state machine. */
void VFD_Service(VFD *vfd);

/* Copy exactly eight printable ASCII characters and schedule a render. */
bool VFD_SetCharacters(VFD *vfd, const char characters[VFD_CHARACTER_COUNT]);

bool VFD_IsReady(const VFD *vfd);
bool VFD_HasError(const VFD *vfd);

#ifdef __cplusplus
}
#endif

#endif //WHOSONFIRST_VFD_H
