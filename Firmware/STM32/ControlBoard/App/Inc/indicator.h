#ifndef INDICATOR_H
#define INDICATOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INDICATOR_OFF = 0,
    INDICATOR_ID,
    INDICATOR_ERROR,
    INDICATOR_PREPARE,
    INDICATOR_STRIKE,
    INDICATOR_SOLVED,
    INDICATOR_MODE_COUNT,
} IndicatorMode;

void Indicator_Init(void);

void Indicator_Service(void);

void Indicator_Set(IndicatorMode mode);

#ifdef __cplusplus
}
#endif

#endif //INDICATOR_H
