#ifndef EDGEWORK_H
#define	EDGEWORK_H

typedef uint8_t indicator_t;

#define INDICATOR_SND ((indicator_t) 0)
#define INDICATOR_CLR ((indicator_t) 1)
#define INDICATOR_CAR ((indicator_t) 2)
#define INDICATOR_IND ((indicator_t) 3)
#define INDICATOR_FRQ ((indicator_t) 4)
#define INDICATOR_SIG ((indicator_t) 5)
#define INDICATOR_NSA ((indicator_t) 6)
#define INDICATOR_MSA ((indicator_t) 7)
#define INDICATOR_TRN ((indicator_t) 8)
#define INDICATOR_BOB ((indicator_t) 9)
#define INDICATOR_FRK ((indicator_t) 10)

#define INDICATOR_MAX INDICATOR_FRK

typedef uint8_t port_t;

#define PORT_DVI        ((port_t) 0)
#define PORT_PARALLEL   ((port_t) 1)
#define PORT_PS2        ((port_t) 2)
#define PORT_RJ45       ((port_t) 3)
#define PORT_SERIAL     ((port_t) 4)
#define PORT_RCA        ((port_t) 5)

#define PORT_MAX        PORT_RCA

void edgework_generate(uint32_t seed, uint8_t difficulty);
void edgework_display(void);
bool edgework_indicator_present(indicator_t ind, bool lit);
uint8_t edgework_battery_count(void);
bool edgework_serial_vowel(void);
uint8_t edgework_serial_last_digit(void);
bool edgework_port_present(port_t port);

#endif	/* EDGEWORK_H */
