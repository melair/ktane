#ifndef BUZZER_H
#define	BUZZER_H

void buzzer_initialise(void);
void buzzer_on(uint8_t volume, uint16_t frequency);
void buzzer_on_timed(uint8_t volume, uint16_t frequency, uint16_t duration);
void buzzer_off(void);
void buzzer_service(void);
uint8_t buzzer_get_volume(void);
void buzzer_set_volume(uint8_t new_vol);

#define BUZZER_FREQ_G4_SHARP    415
#define BUZZER_FREQ_A4          440
#define BUZZER_FREQ_D5_SHARP    622
#define BUZZER_FREQ_E5          659
#define BUZZER_FREQ_F5_SHARP    740
#define BUZZER_FREQ_G5          784
#define BUZZER_FREQ_G5_SHARP    831
#define BUZZER_FREQ_C6          1047

#define BUZZER_FREQ_A6_SHARP    1865
#define BUZZER_FREQ_C7_SHARP    2217
#define BUZZER_FREQ_B8          7902

#define BUZZER_DEFAULT_VOLUME   255
#define BUZZER_DEFAULT_FREQUENCY BUZZER_FREQ_A6_SHARP

#endif	/* BUZZER_H */
