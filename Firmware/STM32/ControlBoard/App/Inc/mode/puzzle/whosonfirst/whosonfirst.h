#ifndef WHOSONFIRST_H
#define WHOSONFIRST_H

#include <stdint.h>
#include "display/epaper/epaper.h"
#include "input_manager/input_manager.h"
#include "mode_fsm.h"
#include "mode/puzzle/whosonfirst/touch.h"
#include "mode/puzzle/whosonfirst/vfd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WHOSONFIRST_EPAPER_WIDTH 400U
#define WHOSONFIRST_EPAPER_HEIGHT 300U
#define WHOSONFIRST_EPAPER_FRAMEBUFFER_SIZE \
    (((WHOSONFIRST_EPAPER_WIDTH + 7U) / 8U) * WHOSONFIRST_EPAPER_HEIGHT)

/* Reserved for the Who's on First puzzle state. */
typedef struct {
    VFD vfd;
    uint32_t startup_message_until_ms;
    bool startup_test_card_started;
    bool epaper_initialized;
    bool epaper_clear_pending;
    Touch touch;
    IM_EventQueue touch_queue;
    struct {
        Epaper display;
        uint8_t black[WHOSONFIRST_EPAPER_FRAMEBUFFER_SIZE];
    } epaper;
} WhosOnFirst_Data;

extern Mode_Definition whosonfirst_mode;

#ifdef __cplusplus
}
#endif

#endif //WHOSONFIRST_H
