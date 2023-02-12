#ifndef KEYS_H
#define	KEYS_H

void keys_initialise(void);

#define KEYS_MIN_INTERVAL 80
#define KEYS_MAX_INTERVAL 120
#define KEYS_LIGHT_WARNING 15

typedef struct {
    uint8_t minutes;
    uint8_t seconds;
    uint8_t key;
} mode_keys_t;

#endif	/* KEYS_H */
