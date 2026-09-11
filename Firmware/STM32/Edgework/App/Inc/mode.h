#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "edgework/data.h"
#include "fsm/fsm.h"

#ifdef __cplusplus
extern "C" {



#endif

typedef uint8_t EdgeworkMode;

typedef enum {
    EDGEWORK_MODE_STATE_INIT = 0,
    EDGEWORK_MODE_STATE_STARTUP,
    EDGEWORK_MODE_STATE_IDLE,
    EDGEWORK_MODE_STATE_BLANK,
    EDGEWORK_MODE_STATE_DISPLAY,
    EDGEWORK_MODE_STATE_COUNT,
} EdgeworkMode_State;

typedef struct {
    void (*enter)(FSM *fsm);
    void (*service)(FSM *fsm);
    bool (*service_predicate)(FSM *fsm);
    void (*exit)(FSM *fsm);
} Mode_Callbacks;

typedef struct Mode_Definition {
    Mode_Callbacks *state_callbacks;
    void (*always_service)(void);
} Mode_Definition;

#define MODE_SERIAL      0x00U
#define MODE_INDICATOR   0x01U
#define MODE_BATTERY     0x02U
#define MODE_PORTS       0x03U
#define MODE_2FA         0x04U
#define MODE_CONTROLLER  0xfeU
#define MODE_UNKNOWN     0xffU

#include "mode/battery/battery.h"
#include "mode/controller/controller.h"
#include "mode/indicator/indicator.h"
#include "mode/ports/ports.h"
#include "mode/serial/serial.h"
#include "mode/twofa/twofa.h"

typedef struct {
    edgework_state_t current_state;
    edgework_state_t desired_state;
    bool dirty;

    union {
        Serial_Data serial;
        Indicator_Data indicator;
        Battery_Data battery;
        Ports_Data ports;
        TwoFA_Data twofa;
        Controller_Data controller;
    } mode;
} Mode_Data;

extern Mode_Data mode_data;

bool Mode_Init(void);

void Mode_Service(void);

EdgeworkMode Mode_Get(void);

bool Mode_Ready(void);

edgework_state_t Mode_State(void);

bool Mode_Set(EdgeworkMode mode);

bool Mode_Blank(void);

bool Mode_Display(edgework_state_t state);

#ifdef __cplusplus
}
#endif

#endif // MODE_H
