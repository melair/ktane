#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IM_QUEUE_SIZE 8
#define IM_NO_COLUMN 0xFF
#define IM_INVALID_HANDLE 0xFF

typedef uint8_t IM_EventMask;
typedef uint8_t IM_Handle;

typedef enum {
    IM_EVENT_DOWN = 1u << 0,
    IM_EVENT_HELD = 1u << 1,
    IM_EVENT_UP = 1u << 2,
    IM_EVENT_ROTARY_DELTA = 1u << 3,
    IM_EVENT_ANALOGUE = 1u << 4,
} IM_EventType;

typedef struct {
    IM_Handle handle;
    uint8_t channel;
    IM_EventType event;
    uint32_t timestamp_ms;
    uint32_t duration_ms;
    int16_t delta;
    uint16_t value;
} IM_Event;

typedef struct {
    IM_Event events[IM_QUEUE_SIZE];
    uint8_t read;
    uint8_t write;
    uint8_t count;
    uint8_t dropped;
} IM_EventQueue;

typedef struct {
    bool initialized;
    bool stable_down;
    bool last_read_down;
    uint32_t last_read_change_ms;
    uint32_t down_since_ms;
    uint32_t next_held_event_ms;
} IM_DigitalChannelState;

typedef struct {
    IM_DigitalChannelState *channels;
    uint32_t next_scan_ms;
    uint8_t active_col;
    bool column_strobed;
} IM_DigitalInputState;

typedef struct {
    const GPIO_PinDef *rows;
    uint8_t row_count;

    const GPIO_PinDef *cols;
    uint8_t col_count;

    IM_EventQueue *queue;
    IM_EventMask event_mask;
    IM_DigitalInputState *state;

    uint16_t scan_period_ms;
    uint16_t debounce_ms;
    uint16_t held_event_interval_ms;

    bool enable_internal_pullups;
    bool active_high;
} IM_DigitalInputConfig;

typedef struct IM_RotaryHardware IM_RotaryHardware;

typedef struct {
    IM_Handle input_handle;
    uint16_t last_count;
    int16_t residual_counts;
} IM_RotaryEncoderState;

typedef struct {
    const IM_RotaryHardware *hardware;
    IM_EventQueue *queue;
    IM_EventMask event_mask;
    IM_RotaryEncoderState *state;
    bool invert_direction;
    bool enable_internal_pullups;
    uint8_t counts_per_detent;
} IM_RotaryEncoderConfig;

typedef struct {
    uint16_t value;
    uint16_t last_published_value;
    bool baseline_valid;
} IM_AnalogueChannelState;

typedef struct {
    GPIO_PinDef pin;
    uint32_t adc_channel;
} IM_AnaloguePinConfig;

typedef struct {
    IM_AnalogueChannelState *channels;
    uint32_t next_scan_ms;
    uint8_t active_pin;
    bool scan_active;
} IM_AnalogueInputState;

typedef struct {
    const IM_AnaloguePinConfig *pins;
    uint8_t pin_count;
    IM_EventQueue *queue;
    IM_AnalogueInputState *state;
    uint16_t scan_interval_ms;
    uint16_t change_threshold;
} IM_AnalogueInputConfig;

void IM_Init(void);

void IM_Service(void);

bool IM_EventQueue_Read(IM_EventQueue *queue, IM_Event *event);

void IM_EventQueue_Clear(IM_EventQueue *queue);

bool IM_DigitalChannel_Get(const IM_DigitalInputConfig *config, uint8_t row, uint8_t col);

IM_Handle IM_RegisterDigital(const IM_DigitalInputConfig *config);

IM_Handle IM_RegisterRotaryEncoder(const IM_RotaryEncoderConfig *config);

IM_Handle IM_RegisterAnalogue(const IM_AnalogueInputConfig *config);

#ifdef __cplusplus
}
#endif

#endif //INPUT_MANAGER_H
