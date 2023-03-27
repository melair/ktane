#ifndef SERIAL_H
#define	SERIAL_H

/* Length of serial number. */
#define SERIAL_NUMBER_LENGTH 4

void serial_initialise(void);
uint32_t serial_get(void);

#endif	/* SERIAL_H */

