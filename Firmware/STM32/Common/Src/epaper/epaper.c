#include "epaper/epaper.h"

#include <stddef.h>
#include <string.h>

#define SSD1680_CMD_DRIVER_OUTPUT_CONTROL        0x01U
#define SSD1680_CMD_DEEP_SLEEP                   0x10U
#define SSD1680_CMD_DATA_ENTRY_MODE              0x11U
#define SSD1680_CMD_SW_RESET                     0x12U
#define SSD1680_CMD_TEMPERATURE_SENSOR_CONTROL   0x18U
#define SSD1680_CMD_MASTER_ACTIVATION            0x20U
#define SSD1680_CMD_DISPLAY_UPDATE_CONTROL_1     0x21U
#define SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2     0x22U
#define SSD1680_CMD_WRITE_RAM_BW                 0x24U
#define SSD1680_CMD_WRITE_RAM_RED                0x26U
#define SSD1680_CMD_BORDER_WAVEFORM_CONTROL      0x3CU
#define SSD1680_CMD_SET_RAM_X                    0x44U
#define SSD1680_CMD_SET_RAM_Y                    0x45U
#define SSD1680_CMD_SET_RAM_X_COUNTER            0x4EU
#define SSD1680_CMD_SET_RAM_Y_COUNTER            0x4FU

#define EPAPER_RESET_DELAY_MS 10U
#define EPAPER_TEST_CARD_MINOR_GRID_PX 5U
#define EPAPER_TEST_CARD_MAJOR_GRID_PX 10U
#define EPAPER_TEST_CARD_DOT_PERIOD_PX 3U
#define EPAPER_TEST_CARD_ORIGIN_SIZE_PX 20U

static const uint8_t command_driver_output_control = SSD1680_CMD_DRIVER_OUTPUT_CONTROL;
static const uint8_t command_deep_sleep = SSD1680_CMD_DEEP_SLEEP;
static const uint8_t command_data_entry_mode = SSD1680_CMD_DATA_ENTRY_MODE;
static const uint8_t command_sw_reset = SSD1680_CMD_SW_RESET;
static const uint8_t command_temperature_sensor_control = SSD1680_CMD_TEMPERATURE_SENSOR_CONTROL;
static const uint8_t command_master_activation = SSD1680_CMD_MASTER_ACTIVATION;
static const uint8_t command_display_update_control_1 = SSD1680_CMD_DISPLAY_UPDATE_CONTROL_1;
static const uint8_t command_display_update_control_2 = SSD1680_CMD_DISPLAY_UPDATE_CONTROL_2;
static const uint8_t command_write_ram_bw = SSD1680_CMD_WRITE_RAM_BW;
static const uint8_t command_write_ram_red = SSD1680_CMD_WRITE_RAM_RED;
static const uint8_t command_border_waveform_control = SSD1680_CMD_BORDER_WAVEFORM_CONTROL;
static const uint8_t command_set_ram_x = SSD1680_CMD_SET_RAM_X;
static const uint8_t command_set_ram_y = SSD1680_CMD_SET_RAM_Y;
static const uint8_t command_set_ram_x_counter = SSD1680_CMD_SET_RAM_X_COUNTER;
static const uint8_t command_set_ram_y_counter = SSD1680_CMD_SET_RAM_Y_COUNTER;

typedef enum {
    EPAPER_STATE_IDLE = 0,
    EPAPER_STATE_RESET_ASSERT,
    EPAPER_STATE_RESET_RELEASE,
    EPAPER_STATE_WAIT_RESET,
    EPAPER_STATE_SW_RESET_SEND,
    EPAPER_STATE_WAIT_SW_RESET,
    EPAPER_STATE_CONFIGURE_SEND,
    EPAPER_STATE_WAIT_CONFIGURE,
    EPAPER_STATE_WINDOW_SEND,
    EPAPER_STATE_BW_COMMAND_SEND,
    EPAPER_STATE_BW_DATA_SEND,
    EPAPER_STATE_RED_COMMAND_SEND,
    EPAPER_STATE_RED_DATA_SEND,
    EPAPER_STATE_UPDATE_SEND,
    EPAPER_STATE_WAIT_UPDATE,
    EPAPER_STATE_SLEEP_SEND,
    EPAPER_STATE_ERROR,
} Epaper_State;

static void epaper_idle_service(FSM *fsm);
static void epaper_reset_assert_enter(FSM *fsm);
static void epaper_reset_release_enter(FSM *fsm);
static void epaper_wait_reset_service(FSM *fsm);
static void epaper_sw_reset_send_enter(FSM *fsm);
static void epaper_wait_sw_reset_service(FSM *fsm);
static void epaper_configure_send_enter(FSM *fsm);
static void epaper_wait_configure_service(FSM *fsm);
static void epaper_window_send_enter(FSM *fsm);
static void epaper_bw_command_send_enter(FSM *fsm);
static void epaper_bw_data_send_enter(FSM *fsm);
static void epaper_red_command_send_enter(FSM *fsm);
static void epaper_red_data_send_enter(FSM *fsm);
static void epaper_update_send_enter(FSM *fsm);
static void epaper_wait_update_service(FSM *fsm);
static void epaper_sleep_send_enter(FSM *fsm);

static const FSM_State epaper_states[] = {
    [EPAPER_STATE_IDLE] = {.service = epaper_idle_service,
                           .next_mask = FSM_NEXT(EPAPER_STATE_RESET_ASSERT)},
    [EPAPER_STATE_RESET_ASSERT] = {.enter = epaper_reset_assert_enter,
                                   .next_mask = FSM_NEXT(EPAPER_STATE_RESET_RELEASE)},
    [EPAPER_STATE_RESET_RELEASE] = {.enter = epaper_reset_release_enter,
                                    .next_mask = FSM_NEXT(EPAPER_STATE_WAIT_RESET)},
    [EPAPER_STATE_WAIT_RESET] = {.service = epaper_wait_reset_service,
                                 .next_mask = FSM_NEXT(EPAPER_STATE_SW_RESET_SEND)},
    [EPAPER_STATE_SW_RESET_SEND] = {.enter = epaper_sw_reset_send_enter,
                                    .next_mask = FSM_NEXT(EPAPER_STATE_WAIT_SW_RESET) |
                                                 FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_WAIT_SW_RESET] = {.service = epaper_wait_sw_reset_service,
                                    .next_mask = FSM_NEXT(EPAPER_STATE_CONFIGURE_SEND)},
    [EPAPER_STATE_CONFIGURE_SEND] = {.enter = epaper_configure_send_enter,
                                     .next_mask = FSM_NEXT(EPAPER_STATE_WAIT_CONFIGURE) |
                                                  FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_WAIT_CONFIGURE] = {.service = epaper_wait_configure_service,
                                     .next_mask = FSM_NEXT(EPAPER_STATE_WINDOW_SEND)},
    [EPAPER_STATE_WINDOW_SEND] = {.enter = epaper_window_send_enter,
                                  .next_mask = FSM_NEXT(EPAPER_STATE_BW_COMMAND_SEND) |
                                               FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_BW_COMMAND_SEND] = {.enter = epaper_bw_command_send_enter,
                                      .next_mask = FSM_NEXT(EPAPER_STATE_BW_DATA_SEND) |
                                                   FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_BW_DATA_SEND] = {.enter = epaper_bw_data_send_enter,
                                   .next_mask = FSM_NEXT(EPAPER_STATE_RED_COMMAND_SEND) |
                                                FSM_NEXT(EPAPER_STATE_UPDATE_SEND) |
                                                FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_RED_COMMAND_SEND] = {.enter = epaper_red_command_send_enter,
                                       .next_mask = FSM_NEXT(EPAPER_STATE_RED_DATA_SEND) |
                                                    FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_RED_DATA_SEND] = {.enter = epaper_red_data_send_enter,
                                    .next_mask = FSM_NEXT(EPAPER_STATE_UPDATE_SEND) |
                                                 FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_UPDATE_SEND] = {.enter = epaper_update_send_enter,
                                  .next_mask = FSM_NEXT(EPAPER_STATE_WAIT_UPDATE) |
                                               FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_WAIT_UPDATE] = {.service = epaper_wait_update_service,
                                  .next_mask = FSM_NEXT(EPAPER_STATE_SLEEP_SEND)},
    [EPAPER_STATE_SLEEP_SEND] = {.enter = epaper_sleep_send_enter,
                                 .next_mask = FSM_NEXT(EPAPER_STATE_IDLE) |
                                              FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_ERROR] = {.next_mask = 0U},
};

static Epaper *epaper_from_fsm(FSM *fsm) {
    return fsm->context;
}

static bool busy_is_ready(const Epaper *epaper) {
    return HAL_GPIO_ReadPin(epaper->config.busy.port, epaper->config.busy.pin) == GPIO_PIN_RESET;
}

static void dc_command(void *context) {
    Epaper *epaper = context;
    HAL_GPIO_WritePin(epaper->config.dc.port, epaper->config.dc.pin, GPIO_PIN_RESET);
}

static void dc_data(void *context) {
    Epaper *epaper = context;
    HAL_GPIO_WritePin(epaper->config.dc.port, epaper->config.dc.pin, GPIO_PIN_SET);
}

static void sequence_complete(void *context, bool success) {
    Epaper *epaper = context;
    (void) FSM_Transition(&epaper->fsm,
                          success ? epaper->sequence_complete_state : EPAPER_STATE_ERROR);
}

static bool queue_steps(Epaper *epaper, uint16_t count, Epaper_State complete_state) {
    epaper->sequence_complete_state = (uint8_t) complete_state;
    if (!SPI_Sequence_Init(&epaper->sequence, &epaper->spi_template, epaper->steps, count,
                           epaper, sequence_complete)) {
        return false;
    }

    SPI_Transaction *transaction = SPI_Sequence_Start(&epaper->sequence);
    if (transaction == NULL) {
        return false;
    }

    SPI_Queue(transaction);
    return true;
}

static void set_step(Epaper *epaper, uint16_t index, const void *data, uint16_t size, bool is_data) {
    epaper->steps[index] = (SPI_SequenceStep) {
        .data = data,
        .size = size,
        .prepare = is_data ? dc_data : dc_command,
    };
}

static void queue_or_error(FSM *fsm, uint16_t count, Epaper_State complete_state) {
    Epaper *epaper = epaper_from_fsm(fsm);
    if (!queue_steps(epaper, count, complete_state)) {
        (void) FSM_Transition(fsm, EPAPER_STATE_ERROR);
    }
}

static void epaper_idle_service(FSM *fsm) {
    (void) fsm;
}

static void epaper_reset_assert_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    HAL_GPIO_WritePin(epaper->config.reset.port, epaper->config.reset.pin, GPIO_PIN_RESET);
    (void) FSM_TransitionIn(fsm, EPAPER_STATE_RESET_RELEASE, EPAPER_RESET_DELAY_MS);
}

static void epaper_reset_release_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    HAL_GPIO_WritePin(epaper->config.reset.port, epaper->config.reset.pin, GPIO_PIN_SET);
    (void) FSM_TransitionIn(fsm, EPAPER_STATE_WAIT_RESET, EPAPER_RESET_DELAY_MS);
}

static void epaper_wait_reset_service(FSM *fsm) {
    if (busy_is_ready(epaper_from_fsm(fsm))) {
        (void) FSM_Transition(fsm, EPAPER_STATE_SW_RESET_SEND);
    }
}

static void epaper_sw_reset_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    set_step(epaper, 0U, &command_sw_reset, 1U, false);
    queue_or_error(fsm, 1U, EPAPER_STATE_WAIT_SW_RESET);
}

static void epaper_wait_sw_reset_service(FSM *fsm) {
    if (busy_is_ready(epaper_from_fsm(fsm))) {
        (void) FSM_Transition(fsm, EPAPER_STATE_CONFIGURE_SEND);
    }
}

static void epaper_configure_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    const uint16_t height = epaper->config.height - 1U;
    static const uint8_t data_entry_mode = 0x03U;
    static const uint8_t border_waveform = 0x05U;
    static const uint8_t update_control_1[] = {0x00U, 0x80U};
    static const uint8_t temperature_sensor = 0x80U;
    static const uint8_t update_control_2 = 0xB1U;

    epaper->driver_output_data[0] = (uint8_t) height;
    epaper->driver_output_data[1] = (uint8_t) (height >> 8U);
    epaper->driver_output_data[2] = 0x00U;

    set_step(epaper, 0U, &command_driver_output_control, 1U, false);
    set_step(epaper, 1U, epaper->driver_output_data, sizeof(epaper->driver_output_data), true);
    set_step(epaper, 2U, &command_data_entry_mode, 1U, false);
    set_step(epaper, 3U, &data_entry_mode, 1U, true);
    set_step(epaper, 4U, &command_border_waveform_control, 1U, false);
    set_step(epaper, 5U, &border_waveform, 1U, true);
    set_step(epaper, 6U, &command_display_update_control_1, 1U, false);
    set_step(epaper, 7U, update_control_1, sizeof(update_control_1), true);
    set_step(epaper, 8U, &command_temperature_sensor_control, 1U, false);
    set_step(epaper, 9U, &temperature_sensor, 1U, true);
    set_step(epaper, 10U, &command_display_update_control_2, 1U, false);
    set_step(epaper, 11U, &update_control_2, 1U, true);
    set_step(epaper, 12U, &command_master_activation, 1U, false);
    queue_or_error(fsm, 13U, EPAPER_STATE_WAIT_CONFIGURE);
}

static void epaper_wait_configure_service(FSM *fsm) {
    if (busy_is_ready(epaper_from_fsm(fsm))) {
        (void) FSM_Transition(fsm, EPAPER_STATE_WINDOW_SEND);
    }
}

static void epaper_window_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    const uint16_t height = epaper->config.height - 1U;
    epaper->ram_x_data[0] = 0U;
    epaper->ram_x_data[1] = (uint8_t) (epaper->stride - 1U);
    epaper->ram_y_data[0] = 0U;
    epaper->ram_y_data[1] = 0U;
    epaper->ram_y_data[2] = (uint8_t) height;
    epaper->ram_y_data[3] = (uint8_t) (height >> 8U);

    set_step(epaper, 0U, &command_set_ram_x, 1U, false);
    set_step(epaper, 1U, epaper->ram_x_data, sizeof(epaper->ram_x_data), true);
    set_step(epaper, 2U, &command_set_ram_y, 1U, false);
    set_step(epaper, 3U, epaper->ram_y_data, sizeof(epaper->ram_y_data), true);
    set_step(epaper, 4U, &command_set_ram_x_counter, 1U, false);
    set_step(epaper, 5U, epaper->ram_x_data, 1U, true);
    set_step(epaper, 6U, &command_set_ram_y_counter, 1U, false);
    set_step(epaper, 7U, epaper->ram_y_data, 2U, true);
    queue_or_error(fsm, 8U, EPAPER_STATE_BW_COMMAND_SEND);
}

static void epaper_bw_command_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    set_step(epaper, 0U, &command_write_ram_bw, 1U, false);
    queue_or_error(fsm, 1U, EPAPER_STATE_BW_DATA_SEND);
}

static void epaper_bw_data_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    set_step(epaper, 0U, epaper->config.black_framebuffer,
             (uint16_t) ((uint32_t) epaper->stride * epaper->config.height), true);
    queue_or_error(fsm, 1U, epaper->config.red_framebuffer != NULL
                                ? EPAPER_STATE_RED_COMMAND_SEND
                                : EPAPER_STATE_UPDATE_SEND);
}

static void epaper_red_command_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    set_step(epaper, 0U, &command_write_ram_red, 1U, false);
    queue_or_error(fsm, 1U, EPAPER_STATE_RED_DATA_SEND);
}

static void epaper_red_data_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    set_step(epaper, 0U, epaper->config.red_framebuffer,
             (uint16_t) ((uint32_t) epaper->stride * epaper->config.height), true);
    queue_or_error(fsm, 1U, EPAPER_STATE_UPDATE_SEND);
}

static void epaper_update_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    static const uint8_t update_control = 0xF7U;
    set_step(epaper, 0U, &command_display_update_control_2, 1U, false);
    set_step(epaper, 1U, &update_control, 1U, true);
    set_step(epaper, 2U, &command_master_activation, 1U, false);
    queue_or_error(fsm, 3U, EPAPER_STATE_WAIT_UPDATE);
}

static void epaper_wait_update_service(FSM *fsm) {
    if (busy_is_ready(epaper_from_fsm(fsm))) {
        (void) FSM_Transition(fsm, EPAPER_STATE_SLEEP_SEND);
    }
}

static void epaper_sleep_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    static const uint8_t deep_sleep = 0x01U;
    set_step(epaper, 0U, &command_deep_sleep, 1U, false);
    set_step(epaper, 1U, &deep_sleep, 1U, true);
    queue_or_error(fsm, 2U, EPAPER_STATE_IDLE);
}

bool Epaper_Init(Epaper *epaper, const Epaper_Config *config) {
    if ((epaper == NULL) || (config == NULL) || (config->width == 0U) ||
        (config->height == 0U) || (config->window.width == 0U) ||
        (config->window.height == 0U) || (config->window.x >= config->width) ||
        (config->window.y >= config->height) ||
        (config->window.width > (config->width - config->window.x)) ||
        (config->window.height > (config->height - config->window.y)) ||
        (config->black_framebuffer == NULL) ||
        (config->cs_port == NULL) || (config->cs_pin == 0U) ||
        (config->dc.port == NULL) || (config->dc.pin == 0U) ||
        (config->reset.port == NULL) || (config->reset.pin == 0U) ||
        (config->busy.port == NULL) || (config->busy.pin == 0U)) {
        return false;
    }

    *epaper = (Epaper) {0};
    epaper->config = *config;
    epaper->stride = (uint16_t) ((config->width + 7U) / 8U);
    const size_t framebuffer_size = (size_t) epaper->stride * config->height;

    /* Initialise the complete native panel, including the bezel-masked area, to white. */
    memset(epaper->config.black_framebuffer, 0xff, framebuffer_size);
    if (epaper->config.red_framebuffer != NULL) {
        memset(epaper->config.red_framebuffer, 0x00, framebuffer_size);
    }

    epaper->spi_template = (SPI_Transaction) {
        .bits = 8U,
        .baud = config->baud,
        .operation = SPI_OPERATION_WRITE,
        .cs_port = config->cs_port,
        .cs_pin = config->cs_pin,
        .lsb_first = false,
        .cke = false,
        .ckp = false,
    };

    GPIO_InitTypeDef dc_gpio_init = {
        .Pin = config->dc.pin,
        /*
         * The panel control lines are shared with removable display hardware.
         * Only actively pull them low; releasing a line lets the MCU-side
         * pull-up establish logic high and avoids sourcing current into a
         * powered-down or faulted panel.
         */
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    GPIO_InitTypeDef reset_gpio_init = {
        .Pin = config->reset.pin,
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    GPIO_InitTypeDef busy_gpio_init = {
        .Pin = config->busy.pin,
        .Mode = GPIO_MODE_INPUT,
        /* BUSY is driven by the e-paper controller. */
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };

    HAL_GPIO_WritePin(config->dc.port, config->dc.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(config->reset.port, config->reset.pin, GPIO_PIN_SET);
    HAL_GPIO_Init(config->dc.port, &dc_gpio_init);
    HAL_GPIO_Init(config->reset.port, &reset_gpio_init);
    HAL_GPIO_Init(config->busy.port, &busy_gpio_init);

    return FSM_Init(&epaper->fsm, epaper_states, EPAPER_STATE_IDLE, epaper);
}

void Epaper_Service(Epaper *epaper) {
    if (epaper != NULL) {
        FSM_Service(&epaper->fsm);
    }
}

bool Epaper_IsReady(const Epaper *epaper) {
    return (epaper != NULL) && (epaper->fsm.current_id == EPAPER_STATE_IDLE) &&
           !epaper->fsm.transition_pending;
}

bool Epaper_Refresh(Epaper *epaper) {
    if (!Epaper_IsReady(epaper)) {
        return false;
    }

    return FSM_Transition(&epaper->fsm, EPAPER_STATE_RESET_ASSERT);
}

bool Epaper_SetRotation(Epaper *epaper, Epaper_Rotation rotation) {
    if ((epaper == NULL) || (rotation > EPAPER_ROTATION_270) || !Epaper_IsReady(epaper)) {
        return false;
    }

    epaper->rotation = rotation;
    return true;
}

uint16_t Epaper_Width(const Epaper *epaper) {
    if (epaper == NULL) {
        return 0U;
    }

    return (epaper->rotation == EPAPER_ROTATION_90 || epaper->rotation == EPAPER_ROTATION_270)
               ? epaper->config.window.height
               : epaper->config.window.width;
}

uint16_t Epaper_Height(const Epaper *epaper) {
    if (epaper == NULL) {
        return 0U;
    }

    return (epaper->rotation == EPAPER_ROTATION_90 || epaper->rotation == EPAPER_ROTATION_270)
               ? epaper->config.window.width
               : epaper->config.window.height;
}

static bool map_pixel(const Epaper *epaper, uint16_t x, uint16_t y,
                      uint16_t *physical_x, uint16_t *physical_y) {
    if ((epaper == NULL) || (x >= Epaper_Width(epaper)) || (y >= Epaper_Height(epaper))) {
        return false;
    }

    switch (epaper->rotation) {
        case EPAPER_ROTATION_0:
            *physical_x = epaper->config.window.x + x;
            *physical_y = epaper->config.window.y + y;
            break;
        case EPAPER_ROTATION_90:
            *physical_x = epaper->config.window.x + epaper->config.window.width - 1U - y;
            *physical_y = epaper->config.window.y + x;
            break;
        case EPAPER_ROTATION_180:
            *physical_x = epaper->config.window.x + epaper->config.window.width - 1U - x;
            *physical_y = epaper->config.window.y + epaper->config.window.height - 1U - y;
            break;
        case EPAPER_ROTATION_270:
            *physical_x = epaper->config.window.x + y;
            *physical_y = epaper->config.window.y + epaper->config.window.height - 1U - x;
            break;
        default:
            return false;
    }

    return true;
}

void Epaper_SetPixel(Epaper *epaper, uint16_t x, uint16_t y, Epaper_Colour colour) {
    uint16_t physical_x;
    uint16_t physical_y;
    if ((epaper == NULL) || (colour > EPAPER_COLOUR_RED) ||
        !map_pixel(epaper, x, y, &physical_x, &physical_y) ||
        ((colour == EPAPER_COLOUR_RED) && (epaper->config.red_framebuffer == NULL))) {
        return;
    }

    const uint32_t offset = ((uint32_t) physical_y * epaper->stride) + (physical_x / 8U);
    const uint8_t bit = (uint8_t) (0x80U >> (physical_x & 7U));
    if (colour == EPAPER_COLOUR_BLACK) {
        epaper->config.black_framebuffer[offset] &= (uint8_t) ~bit;
    } else {
        epaper->config.black_framebuffer[offset] |= bit;
    }

    if (epaper->config.red_framebuffer != NULL) {
        if (colour == EPAPER_COLOUR_RED) {
            epaper->config.red_framebuffer[offset] |= bit;
        } else {
            epaper->config.red_framebuffer[offset] &= (uint8_t) ~bit;
        }
    }
}

static void epaper_or_pixel(Epaper *epaper, uint16_t x, uint16_t y, Epaper_Colour colour) {
    uint16_t physical_x;
    uint16_t physical_y;
    if ((epaper == NULL) || (colour > EPAPER_COLOUR_RED) ||
        !map_pixel(epaper, x, y, &physical_x, &physical_y) ||
        ((colour == EPAPER_COLOUR_RED) && (epaper->config.red_framebuffer == NULL))) {
        return;
    }

    const uint32_t offset = ((uint32_t) physical_y * epaper->stride) + (physical_x / 8U);
    const uint8_t bit = (uint8_t) (0x80U >> (physical_x & 7U));
    switch (colour) {
        case EPAPER_COLOUR_WHITE:
            epaper->config.black_framebuffer[offset] |= bit;
            if (epaper->config.red_framebuffer != NULL) {
                epaper->config.red_framebuffer[offset] &= (uint8_t) ~bit;
            }
            break;
        case EPAPER_COLOUR_BLACK:
            /* Black is active-low in the SSD1680 black/white plane. */
            epaper->config.black_framebuffer[offset] &= (uint8_t) ~bit;
            break;
        case EPAPER_COLOUR_RED:
            /* Red is active-high in the SSD1680 chromatic plane. */
            epaper->config.red_framebuffer[offset] |= bit;
            break;
        default:
            break;
    }
}

void Epaper_Fill(Epaper *epaper, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 Epaper_Colour colour) {
    if ((epaper == NULL) || (width == 0U) || (height == 0U) ||
        (x >= Epaper_Width(epaper)) || (y >= Epaper_Height(epaper))) {
        return;
    }

    const uint16_t end_x = width > (Epaper_Width(epaper) - x) ? Epaper_Width(epaper) : x + width;
    const uint16_t end_y = height > (Epaper_Height(epaper) - y) ? Epaper_Height(epaper) : y + height;
    for (uint16_t row = y; row < end_y; row++) {
        for (uint16_t col = x; col < end_x; col++) {
            Epaper_SetPixel(epaper, col, row, colour);
        }
    }
}

void Epaper_DrawTestCard(Epaper *epaper) {
    if (epaper == NULL) {
        return;
    }

    const uint16_t width = Epaper_Width(epaper);
    const uint16_t height = Epaper_Height(epaper);
    Epaper_Fill(epaper, 0U, 0U, width, height, EPAPER_COLOUR_WHITE);

    for (uint16_t y = 0U; y < height; y++) {
        for (uint16_t x = 0U; x < width; x++) {
            const bool border_line = (x == 0U) || (y == 0U) || (x == (width - 1U)) ||
                                     (y == (height - 1U));
            const bool major_line = ((x % EPAPER_TEST_CARD_MAJOR_GRID_PX) == 0U) ||
                                    ((y % EPAPER_TEST_CARD_MAJOR_GRID_PX) == 0U);
            const bool vertical_minor_dot =
                ((x % EPAPER_TEST_CARD_MINOR_GRID_PX) == 0U) &&
                ((y % EPAPER_TEST_CARD_DOT_PERIOD_PX) == 0U);
            const bool horizontal_minor_dot =
                ((y % EPAPER_TEST_CARD_MINOR_GRID_PX) == 0U) &&
                ((x % EPAPER_TEST_CARD_DOT_PERIOD_PX) == 0U);

            if (border_line || major_line || vertical_minor_dot || horizontal_minor_dot) {
                Epaper_SetPixel(epaper, x, y, EPAPER_COLOUR_BLACK);
            }
        }
    }

    /* A large block at the logical origin makes display orientation unambiguous. */
    Epaper_Fill(epaper, 0U, 0U, EPAPER_TEST_CARD_ORIGIN_SIZE_PX,
                EPAPER_TEST_CARD_ORIGIN_SIZE_PX, EPAPER_COLOUR_BLACK);
}

void Epaper_CopySprite(Epaper *epaper, uint16_t x, uint16_t y, const uint8_t *sprite,
                       uint16_t width, uint16_t height, Epaper_Colour colour,
                       Epaper_SpriteComposite composite) {
    if ((epaper == NULL) || (sprite == NULL) || (width == 0U) || (height == 0U) ||
        (colour > EPAPER_COLOUR_RED) || (composite > EPAPER_SPRITE_COMPOSITE_OR)) {
        return;
    }

    const uint16_t display_width = Epaper_Width(epaper);
    const uint16_t display_height = Epaper_Height(epaper);
    if ((x >= display_width) || (y >= display_height)) {
        return;
    }

    const uint16_t sprite_stride = (uint16_t) ((width + 7U) / 8U);
    for (uint16_t row = 0U; row < height; row++) {
        if (row >= (display_height - y)) {
            break;
        }
        for (uint16_t col = 0U; col < width; col++) {
            if (col >= (display_width - x)) {
                break;
            }
            const uint8_t source = sprite[((uint32_t) row * sprite_stride) + (col / 8U)];
            if ((source & (uint8_t) (0x80U >> (col & 7U))) != 0U) {
                if (composite == EPAPER_SPRITE_COMPOSITE_SET) {
                    Epaper_SetPixel(epaper, (uint16_t) (x + col), (uint16_t) (y + row), colour);
                } else {
                    epaper_or_pixel(epaper, (uint16_t) (x + col), (uint16_t) (y + row), colour);
                }
            } else if (composite == EPAPER_SPRITE_COMPOSITE_SET) {
                Epaper_SetPixel(epaper, (uint16_t) (x + col), (uint16_t) (y + row),
                                EPAPER_COLOUR_WHITE);
            }
        }
    }
}

static const glyph_t *epaper_font_glyph(const font_t *font, const uint8_t character) {
    if ((font == NULL) || (font->glyphs == NULL) || (font->character_map == NULL) ||
        (character >= font->character_map_count)) {
        return NULL;
    }

    const int16_t index = font->character_map[character];
    if ((index < 0) || ((uint16_t) index >= font->glyph_count)) {
        return NULL;
    }

    return &font->glyphs[index];
}

void Epaper_Text(Epaper *epaper, const uint8_t *text, const uint16_t length,
                 const font_t *font, const Epaper_Window bounds,
                 const Epaper_TextHorizontalAlign horizontal_align,
                 const Epaper_TextVerticalAlign vertical_align, const Epaper_Colour colour) {
    if ((epaper == NULL) || (text == NULL) || (font == NULL) || (font->bitmap == NULL) ||
        (horizontal_align > EPAPER_TEXT_ALIGN_RIGHT) ||
        (vertical_align > EPAPER_TEXT_ALIGN_MIDDLE_BODY) || (colour > EPAPER_COLOUR_RED)) {
        return;
    }

    const uint16_t minimum_height = vertical_align == EPAPER_TEXT_ALIGN_MIDDLE_BODY ?
                                        (uint16_t) font->ascent : font->line_height;
    if ((bounds.width == 0U) || (bounds.height < minimum_height)) {
        return;
    }

    bool has_glyph = false;
    int32_t text_left = 0;
    int32_t text_right = 0;
    int32_t cursor_x = 0;
    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = epaper_font_glyph(font, text[index]);
        if (glyph == NULL) {
            continue;
        }

        const int32_t glyph_left = cursor_x + glyph->x_offset;
        const int32_t glyph_right = glyph_left + glyph->width;
        if (!has_glyph || (glyph_left < text_left)) {
            text_left = glyph_left;
        }
        if (!has_glyph || (glyph_right > text_right)) {
            text_right = glyph_right;
        }
        has_glyph = true;
        cursor_x += glyph->x_advance;
    }

    if (!has_glyph) {
        return;
    }

    const int32_t text_width = text_right - text_left;
    int32_t cursor_start = bounds.x - text_left;
    if (horizontal_align == EPAPER_TEXT_ALIGN_CENTRE) {
        cursor_start += ((int32_t) bounds.width - text_width) / 2;
    } else if (horizontal_align == EPAPER_TEXT_ALIGN_RIGHT) {
        cursor_start += (int32_t) bounds.width - text_width;
    }

    int32_t baseline = bounds.y + font->ascent;
    if (vertical_align == EPAPER_TEXT_ALIGN_MIDDLE) {
        baseline += ((int32_t) bounds.height - font->line_height) / 2;
    } else if (vertical_align == EPAPER_TEXT_ALIGN_BOTTOM) {
        baseline += (int32_t) bounds.height - font->line_height;
    } else if (vertical_align == EPAPER_TEXT_ALIGN_MIDDLE_BODY) {
        baseline += ((int32_t) bounds.height - font->ascent) / 2;
    }

    const int32_t bounds_right = (int32_t) bounds.x + bounds.width;
    const int32_t bounds_bottom = (int32_t) bounds.y + bounds.height;
    cursor_x = cursor_start;

    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = epaper_font_glyph(font, text[index]);
        if (glyph == NULL) {
            continue;
        }

        const int32_t glyph_x = cursor_x + glyph->x_offset;
        const int32_t glyph_y = baseline + glyph->y_offset;
        const uint16_t glyph_stride = (uint16_t) ((glyph->width + 7U) / 8U);
        for (uint16_t row = 0U; row < glyph->height; row++) {
            const int32_t pixel_y = glyph_y + row;
            if ((pixel_y < bounds.y) || (pixel_y >= bounds_bottom)) {
                continue;
            }
            for (uint16_t column = 0U; column < glyph->width; column++) {
                const int32_t pixel_x = glyph_x + column;
                if ((pixel_x < bounds.x) || (pixel_x >= bounds_right)) {
                    continue;
                }
                const uint8_t source = font->bitmap[glyph->data_offset +
                                                     ((uint32_t) row * glyph_stride) +
                                                     (column / 8U)];
                if ((source & (uint8_t) (0x80U >> (column & 7U))) != 0U) {
                    Epaper_SetPixel(epaper, (uint16_t) pixel_x, (uint16_t) pixel_y, colour);
                }
            }
        }

        cursor_x += glyph->x_advance;
    }
}
