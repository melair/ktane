#ifndef WHOSONFIRST_H
#define	WHOSONFIRST_H

void whosonfirst_initialise(void);

#define WHOSONFIRST_WORD_COUNT 6

typedef struct {
    uint8_t stage;

    uint8_t topword;
    uint8_t words[WHOSONFIRST_WORD_COUNT];
} mode_whosonfirst_t;

#endif	/* WHOSONFIRST_H */

