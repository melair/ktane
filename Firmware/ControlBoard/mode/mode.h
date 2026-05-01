#ifndef MODE_H
#define MODE_H

void mode_init(void);
void mode_service(void);

#include <stdint.h>
#include <utils/fsm.h>

#define MODE_TYPE_BLANK   0b00 // Blank modules.
#define MODE_TYPE_OTHER   0b01 // Chassis + Timer
#define MODE_TYPE_PUZZLE  0b10 // Solvable modules
#define MODE_TYPE_NEEDY   0b11 // Needy periodic modules

#define MODE_NAME_LEN 20

#define MODE_STATE_COUNT 10

#define MODE_STATE_INIT 0
#define MODE_STATE_STARTUP 1
#define MODE_STATE_IDLE 2
#define MODE_STATE_ATTRACT 3
#define MODE_STATE_PREPARE 4
#define MODE_STATE_READY 5
#define MODE_STATE_STARTING 6
#define MODE_STATE_RUNNING 7
#define MODE_STATE_OVER 8
#define MODE_STATE_ABORT 9

typedef struct {
  void (*entry)(fsm_t *);
  void (*service)(fsm_t *);
  void (*exit)(fsm_t *);  
} mode_state_func_t;

typedef struct {
  union {
    struct {
      unsigned type :2;
      unsigned id :6;
    };
    uint8_t external_id;
  };
  
  uint8_t name[MODE_NAME_LEN];

  const mode_state_func_t *state_funcs[MODE_STATE_COUNT];
} mode_t;

#endif