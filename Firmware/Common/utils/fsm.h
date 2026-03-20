#ifndef FSM_H
#define FSM_H

#include <stdbool.h>

typedef struct fsm_t fsm_t;
typedef struct fs_t fs_t;

struct fs_t{
    const char *name;

    void (*enter)(fsm_t *);
    void (*service)(fsm_t *);
    void (*exit)(fsm_t *);

    const fs_t *next_states[];
};

struct fsm_t {
    const fs_t *current;
    const fs_t *transition;

    const fs_t *initial;
};

void fsm_init(fsm_t *fsm);
void fsm_service(fsm_t *fsm);
bool fsm_transition(fsm_t *fsm, fs_t *fs);

#endif