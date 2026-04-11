#ifndef INTERRUPT_H
#define	INTERRUPT_H

void int_init(uint24_t base_address);
inline void int_enable(void);
inline void int_disable(void);

#endif