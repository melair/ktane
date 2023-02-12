#ifndef PASSWORD_H
#define	PASSWORD_H

void password_initialise(void);

#define LENGTH          5
#define LETTER_OPTIONS  6
#define ALPHABET        26

typedef struct {
    uint8_t word;
    uint8_t letters[LENGTH][LETTER_OPTIONS];
    uint8_t selected[LENGTH];
} mode_password_t;

#endif	/* PASSWORD_H */
