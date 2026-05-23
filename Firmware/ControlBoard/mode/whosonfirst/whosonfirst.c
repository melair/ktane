#include <xc.h>
#include <hal/pin.h>
#include <utils/time.h>
#include "whosonfirst.h"
#include "../mode.h"
#include <utils/fsm.h>
#include <peripherals/vfd/vfd.h>
#include <utils/mem.h>

vfd_t vfd;

void whosonfirst_common_service(void) {
    vfd_service(&vfd);
}

static void whosonfirst_setup_enter(fsm_t *fsm) {
    vfd_init(&vfd, GPIO_WHOSONFIRST_VFD_EN, GPIO_WHOSONFIRST_VFD_RESET, GPIO_WHOSONFIRST_VFD_CS);

    pin_config(GPIO_WHOSONFIRST_EPAPER_CS, OUTPUT, 0);
    pin_write(GPIO_WHOSONFIRST_EPAPER_CS, true);
    pin_config(GPIO_WHOSONFIRST_EPAPER_DC, OUTPUT, 0);
    pin_config(GPIO_WHOSONFIRST_EPAPER_RES, OUTPUT, 0);
    pin_write(GPIO_WHOSONFIRST_EPAPER_RES, false);
    pin_config(GPIO_WHOSONFIRST_EPAPER_BUSY, INPUT, 0);

    pin_config(GPIO_WHOSONFIRST_TOUCH_RESET, OUTPUT, 0);
    pin_write(GPIO_WHOSONFIRST_TOUCH_RESET, false);
    pin_config(GPIO_WHOSONFIRST_TOUCH_INT, INPUT, 0);

    mode_enable_common_service();
}

static void whosonfirst_startup_enter(fsm_t *fsm) {
    fsm_transition(fsm, &mode_state_idle);
}

const mode_state_func_t whosonfirst_funcs[MODE_STATE_COUNT] = {
    { .enter = &whosonfirst_setup_enter }, // MODE_STATE_SETUP
    { .enter = &whosonfirst_startup_enter }, // MODE_STATE_STARTUP
    {}, // MODE_STATE_IDLE
    {}, // MODE_STATE_ATTRACT
    {}, // MODE_STATE_PREPARE
    {}, // MODE_STATE_READY
    {}, // MODE_STATE_STARTING
    {}, // MODE_STATE_RUNNING
    {}, // MODE_STATE_OVER
    {}, // MODE_STATE_ABORT
};
