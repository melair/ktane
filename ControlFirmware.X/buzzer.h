#ifndef BUZZER_H
#define	BUZZER_H

void buzzer_initialise(void);
void buzzer_on(uint8_t volume, uint16_t frequency);
void buzzer_on_timed(uint16_t duration);
void buzzer_off(void);
void buzzer_service(void);

#endif	/* BUZZER_H */
