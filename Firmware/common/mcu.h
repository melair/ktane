#ifndef MCU_H
#define	MCU_H

void safe_unused_pins(void);
void pps_unlock(void);
void pps_lock(void);
void arbiter_initialise(void);

#endif	/* MCU_H */
