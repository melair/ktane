#include "mode/puzzle/whosonfirst/vfd.h"

#include <string.h>

#define VFD_POWER_STABILIZE_MS 100U
#define VFD_RESET_ASSERT_MS 10U
#define VFD_RESET_READY_MS 10U

#define VFD_COMMAND_DISPLAY_TIMING 0xE0U
#define VFD_DISPLAY_TIMING_8_GRIDS 0x07U
#define VFD_COMMAND_BRIGHTNESS 0xE4U
#define VFD_BRIGHTNESS 0x3FU
#define VFD_COMMAND_DCRAM 0x20U
#define VFD_COMMAND_DISPLAY_ON 0xE8U

typedef enum {
    VFD_STATE_POWER_ENABLE,
    VFD_STATE_RESET_ASSERT,
    VFD_STATE_RESET_RELEASE,
    VFD_STATE_DISPLAY_TIMING,
    VFD_STATE_BRIGHTNESS,
    VFD_STATE_INITIAL_CLEAR,
    VFD_STATE_DISPLAY_ON,
    VFD_STATE_READY,
    VFD_STATE_RENDER_CHARACTERS,
    VFD_STATE_RENDER_DISPLAY_ON,
    VFD_STATE_ERROR,
} VFD_State;

static const uint8_t display_timing[] = {
    VFD_COMMAND_DISPLAY_TIMING,
    VFD_DISPLAY_TIMING_8_GRIDS,
};
static const uint8_t brightness[] = {
    VFD_COMMAND_BRIGHTNESS,
    VFD_BRIGHTNESS,
};
static const uint8_t display_on[] = {VFD_COMMAND_DISPLAY_ON};

static void power_enable_enter(FSM *fsm);
static void reset_assert_enter(FSM *fsm);
static void reset_release_enter(FSM *fsm);
static void display_timing_enter(FSM *fsm);
static void brightness_enter(FSM *fsm);
static void initial_clear_enter(FSM *fsm);
static void display_on_enter(FSM *fsm);
static void ready_service(FSM *fsm);
static void render_characters_enter(FSM *fsm);
static void render_display_on_enter(FSM *fsm);

static const FSM_State vfd_states[] = {
    [VFD_STATE_POWER_ENABLE] = {
        .enter = power_enable_enter,
        .next_mask = FSM_NEXT(VFD_STATE_RESET_ASSERT),
    },
    [VFD_STATE_RESET_ASSERT] = {
        .enter = reset_assert_enter,
        .next_mask = FSM_NEXT(VFD_STATE_RESET_RELEASE),
    },
    [VFD_STATE_RESET_RELEASE] = {
        .enter = reset_release_enter,
        .next_mask = FSM_NEXT(VFD_STATE_DISPLAY_TIMING),
    },
    [VFD_STATE_DISPLAY_TIMING] = {
        .enter = display_timing_enter,
        .next_mask = FSM_NEXT(VFD_STATE_BRIGHTNESS) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_BRIGHTNESS] = {
        .enter = brightness_enter,
        .next_mask = FSM_NEXT(VFD_STATE_INITIAL_CLEAR) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_INITIAL_CLEAR] = {
        .enter = initial_clear_enter,
        .next_mask = FSM_NEXT(VFD_STATE_DISPLAY_ON) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_DISPLAY_ON] = {
        .enter = display_on_enter,
        .next_mask = FSM_NEXT(VFD_STATE_READY) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_READY] = {
        .service = ready_service,
        .next_mask = FSM_NEXT(VFD_STATE_RENDER_CHARACTERS),
    },
    [VFD_STATE_RENDER_CHARACTERS] = {
        .enter = render_characters_enter,
        .next_mask = FSM_NEXT(VFD_STATE_RENDER_DISPLAY_ON) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_RENDER_DISPLAY_ON] = {
        .enter = render_display_on_enter,
        .next_mask = FSM_NEXT(VFD_STATE_READY) | FSM_NEXT(VFD_STATE_ERROR),
    },
    [VFD_STATE_ERROR] = {0},
};

static SPI_Transaction *transfer_complete(SPI_Transaction *transaction) {
    VFD *vfd = transaction->callback_data;
    if (vfd == NULL) {
        return NULL;
    }

    if (transaction->state != SPI_STATE_COMPLETE) {
        (void) FSM_Transition(&vfd->fsm, VFD_STATE_ERROR);
        return NULL;
    }

    switch ((VFD_State) vfd->fsm.current_id) {
        case VFD_STATE_DISPLAY_TIMING:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_BRIGHTNESS);
            break;
        case VFD_STATE_BRIGHTNESS:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_INITIAL_CLEAR);
            break;
        case VFD_STATE_INITIAL_CLEAR:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_DISPLAY_ON);
            break;
        case VFD_STATE_DISPLAY_ON:
        case VFD_STATE_RENDER_DISPLAY_ON:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_READY);
            break;
        case VFD_STATE_RENDER_CHARACTERS:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_RENDER_DISPLAY_ON);
            break;
        default:
            (void) FSM_Transition(&vfd->fsm, VFD_STATE_ERROR);
            break;
    }

    return NULL;
}

static void queue_write(VFD *vfd, const uint8_t *data, const uint16_t size) {
    vfd->transaction.tx_data = (void *) data;
    vfd->transaction.tx_size = size;
    vfd->transaction.state = SPI_STATE_IDLE;
    SPI_Queue(&vfd->transaction);
}

static void power_enable_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    HAL_GPIO_WritePin(vfd->config.enable.port, vfd->config.enable.pin, GPIO_PIN_SET);
    (void) FSM_TransitionIn(fsm, VFD_STATE_RESET_ASSERT, VFD_POWER_STABILIZE_MS);
}

static void reset_assert_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    HAL_GPIO_WritePin(vfd->config.reset.port, vfd->config.reset.pin, GPIO_PIN_RESET);
    (void) FSM_TransitionIn(fsm, VFD_STATE_RESET_RELEASE, VFD_RESET_ASSERT_MS);
}

static void reset_release_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    HAL_GPIO_WritePin(vfd->config.reset.port, vfd->config.reset.pin, GPIO_PIN_SET);
    (void) FSM_TransitionIn(fsm, VFD_STATE_DISPLAY_TIMING, VFD_RESET_READY_MS);
}

static void display_timing_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    queue_write(vfd, display_timing, sizeof(display_timing));
}

static void brightness_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    queue_write(vfd, brightness, sizeof(brightness));
}

static void initial_clear_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    queue_write(vfd, vfd->write_buffer, sizeof(vfd->write_buffer));
}

static void display_on_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    queue_write(vfd, display_on, sizeof(display_on));
}

static void ready_service(FSM *fsm) {
    VFD *vfd = fsm->context;
    if (vfd->render_pending) {
        (void) FSM_Transition(fsm, VFD_STATE_RENDER_CHARACTERS);
    }
}

static void render_characters_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    memcpy(&vfd->write_buffer[1], vfd->pending_characters, VFD_CHARACTER_COUNT);
    vfd->render_pending = false;
    queue_write(vfd, vfd->write_buffer, sizeof(vfd->write_buffer));
}

static void render_display_on_enter(FSM *fsm) {
    VFD *vfd = fsm->context;
    queue_write(vfd, display_on, sizeof(display_on));
}

bool VFD_Init(VFD *vfd, const VFD_Config *config) {
    if (vfd == NULL || config == NULL || config->enable.port == NULL ||
        config->reset.port == NULL || config->cs.port == NULL) {
        return false;
    }

    const VFD_Config saved_config = *config;
    memset(vfd, 0, sizeof(*vfd));
    vfd->config = saved_config;
    memset(vfd->pending_characters, ' ', sizeof(vfd->pending_characters));
    vfd->write_buffer[0] = VFD_COMMAND_DCRAM;
    memset(&vfd->write_buffer[1], ' ', VFD_CHARACTER_COUNT);
    vfd->transaction = (SPI_Transaction) {
        .bits = 8U,
        .baud = SPI_BAUD_250KHZ,
        .operation = SPI_OPERATION_WRITE,
        .cs_port = vfd->config.cs.port,
        .cs_pin = vfd->config.cs.pin,
        .lsb_first = true,
        .cke = false,
        .ckp = false,
        .callback = transfer_complete,
        .callback_data = vfd,
    };

    GPIO_InitTypeDef gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };

    gpio.Pin = vfd->config.enable.pin;
    HAL_GPIO_WritePin(vfd->config.enable.port, gpio.Pin, GPIO_PIN_RESET);
    HAL_GPIO_Init(vfd->config.enable.port, &gpio);

    gpio.Pin = vfd->config.reset.pin;
    HAL_GPIO_WritePin(vfd->config.reset.port, gpio.Pin, GPIO_PIN_RESET);
    HAL_GPIO_Init(vfd->config.reset.port, &gpio);

    gpio.Pin = vfd->config.cs.pin;
    HAL_GPIO_WritePin(vfd->config.cs.port, gpio.Pin, GPIO_PIN_SET);
    HAL_GPIO_Init(vfd->config.cs.port, &gpio);

    vfd->initialized = FSM_Init(&vfd->fsm, vfd_states, VFD_STATE_POWER_ENABLE, vfd);
    return vfd->initialized;
}

void VFD_Service(VFD *vfd) {
    if (vfd != NULL && vfd->initialized) {
        FSM_Service(&vfd->fsm);
    }
}

bool VFD_SetCharacters(VFD *vfd, const char characters[VFD_CHARACTER_COUNT]) {
    if (vfd == NULL || characters == NULL || !vfd->initialized || VFD_HasError(vfd)) {
        return false;
    }

    for (uint8_t index = 0U; index < VFD_CHARACTER_COUNT; index++) {
        const uint8_t character = (uint8_t) characters[index];
        if (character < 0x20U || character > 0x7EU) {
            return false;
        }
    }

    memcpy(vfd->pending_characters, characters, VFD_CHARACTER_COUNT);
    vfd->render_pending = true;
    return true;
}

bool VFD_IsReady(const VFD *vfd) {
    return vfd != NULL && vfd->initialized && vfd->fsm.current_id == VFD_STATE_READY &&
           !vfd->fsm.transition_pending;
}

bool VFD_HasError(const VFD *vfd) {
    return vfd != NULL && vfd->initialized &&
           (vfd->fsm.current_id == VFD_STATE_ERROR ||
            (vfd->fsm.transition_pending && vfd->fsm.transition_id == VFD_STATE_ERROR));
}
