#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the RTC and reset its time/date to the board's startup defaults. */
void RTC_Init(void);

#ifdef __cplusplus
}
#endif

#endif //RTC_H
