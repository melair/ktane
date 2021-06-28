#ifndef STATUS_H
#define	STATUS_H

void status_set(uint8_t);
void status_error(uint8_t);
void status_identify(bool on);
void status_service(void);

#define STATUS_READY        0
#define STATUS_UNSOLVED     1
#define STATUS_SOLVED       2
#define STATUS_IDENTIFY     3
#define STATUS_UNUSED       4

#define ERROR_NONE   0
#define ERROR_LOCAL  1
#define ERROR_REMOTE 2

#endif	/* STATUS_H */

