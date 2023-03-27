#ifndef COMBINATION_H
#define	COMBINATION_H

void combination_initialise(void);

typedef struct {
    int8_t value;
    bool clockwise_traveled;
    bool anticlockwise_traveled;
    uint8_t stage;
    uint8_t expected[3];
    uint8_t entered[2];
    bool even_clockwise;
} mode_combination_t;

#endif	/* COMBINATION_H */
