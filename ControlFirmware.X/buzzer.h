#ifndef BUZZER_H
#define	BUZZER_H

void buzzer_initialise(void);
void buzzer_on(uint8_t volume, uint16_t frequency);
void buzzer_on_timed(uint8_t volume, uint16_t frequency, uint16_t duration);
void buzzer_off(void);
void buzzer_service(void);

#define BUZZER_FREQ_A6_SHARP 1865
#define BUZZER_FREQ_C7_SHARP 2217
#define BUZZER_FREQ_B8       7902

#define BUZZER_DEFAULT_VOLUME 255
#define BUZZER_DEFAULT_FREQUENCY BUZZER_FREQ_A6_SHARP

#endif	/* BUZZER_H */
