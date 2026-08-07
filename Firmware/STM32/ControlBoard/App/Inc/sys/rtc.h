#ifndef RTC_H
#define RTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} RTC_Time;

void RTC_Init(void);

void RTC_GetTime(RTC_Time *time);

void RTC_SetTime(const RTC_Time *time);

#ifdef __cplusplus
}
#endif

#endif //RTC_H
