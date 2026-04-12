#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stdint.h>

#ifndef FSM_MAX_NEXT_STATES
#define FSM_MAX_NEXT_STATES 4
#endif

typedef struct fsm_t fsm_t;
typedef struct fs_t fs_t;

struct fs_t {
  void (*enter)(fsm_t *);
  void (*service)(fsm_t *);
  void (*exit)(fsm_t *);

  const fs_t *next_states[FSM_MAX_NEXT_STATES];
};

struct fsm_t {
  const fs_t *initial;

  const fs_t *current;
  const fs_t *transition;
  uint32_t transition_at;

  void *ctx;
};

void fsm_init(fsm_t *fsm);
void fsm_service(fsm_t *fsm);
bool fsm_transition(fsm_t *fsm, const fs_t *fs);
bool fsm_transition_in(fsm_t *fsm, const fs_t *new_state, uint32_t delayInMs);

#endif