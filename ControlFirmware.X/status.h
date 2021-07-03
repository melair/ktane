#ifndef STATUS_H
#define	STATUS_H

void status_set(uint8_t);
void status_error(uint8_t);
void status_identify(bool on);
void status_service(void);

#define STATUS_IDLE         0
#define STATUS_SETUP        1
#define STATUS_READY        2
#define STATUS_UNSOLVED     3
#define STATUS_SOLVED       4
#define STATUS_UNUSED       5

#define ERROR_NONE            0
#define ERROR_LOCAL           1
#define ERROR_REMOTE_ACTIVE   2
#define ERROR_REMOTE_INACTIVE 3

#endif	/* STATUS_H */

