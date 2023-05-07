#ifndef MUX_H
#define	MUX_H

void mux_initialise(void);
void mux_service(void);
void mux_play(uint16_t id, uint32_t block, uint32_t count);
void mux_stop(uint16_t id);

#endif	/* MUX_H */

