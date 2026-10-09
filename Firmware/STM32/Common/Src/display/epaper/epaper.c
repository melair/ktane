#include "display/epaper/epaper.h"

#include <stddef.h>
#include <string.h>

#define SSD1680_CMD_DRIVER_OUTPUT_CONTROL        0x01U
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
    EPAPER_STATE_COMPLETE,
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
static void epaper_complete_enter(FSM *fsm);
static void epaper_error_enter(FSM *fsm);

static const FSM_State epaper_states[] = {
    [EPAPER_STATE_IDLE] = {.service = epaper_idle_service,
                           .next_mask = FSM_NEXT(EPAPER_STATE_RESET_ASSERT) |
                                        FSM_NEXT(EPAPER_STATE_CONFIGURE_SEND)},
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
                                                FSM_NEXT(EPAPER_STATE_BW_DATA_SEND) |
                                                FSM_NEXT(EPAPER_STATE_COMPLETE) |
                                                FSM_NEXT(EPAPER_STATE_UPDATE_SEND) |
                                                FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_RED_COMMAND_SEND] = {.enter = epaper_red_command_send_enter,
                                       .next_mask = FSM_NEXT(EPAPER_STATE_RED_DATA_SEND) |
                                                    FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_RED_DATA_SEND] = {.enter = epaper_red_data_send_enter,
                                    .next_mask = FSM_NEXT(EPAPER_STATE_RED_DATA_SEND) |
                                                 FSM_NEXT(EPAPER_STATE_UPDATE_SEND) |
                                                 FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_UPDATE_SEND] = {.enter = epaper_update_send_enter,
                                  .next_mask = FSM_NEXT(EPAPER_STATE_WAIT_UPDATE) |
                                               FSM_NEXT(EPAPER_STATE_ERROR)},
    [EPAPER_STATE_WAIT_UPDATE] = {.service = epaper_wait_update_service,
                                  .next_mask = FSM_NEXT(EPAPER_STATE_WINDOW_SEND) |
                                               FSM_NEXT(EPAPER_STATE_COMPLETE)},
    [EPAPER_STATE_COMPLETE] = {.enter = epaper_complete_enter,
                                .next_mask = FSM_NEXT(EPAPER_STATE_IDLE)},
    [EPAPER_STATE_ERROR] = {.enter = epaper_error_enter, .next_mask = 0U},
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
    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = &command_sw_reset,
        .size = 1U,
        .prepare = dc_command,
    };
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
    static const uint8_t ssd1680_border_waveform = 0x05U;
    static const uint8_t ssd1680_update_control_1[] = {0x00U, 0x80U};
    static const uint8_t ssd1683_border_waveform = 0x01U;
    static const uint8_t ssd1683_update_control_1[] = {0x40U, 0x00U};
    static const uint8_t temperature_sensor = 0x80U;
    static const uint8_t update_control_2 = 0xB1U;
    const bool is_ssd1683 = epaper->config.controller == EPAPER_CONTROLLER_SSD1683;
    static const uint8_t partial_border_waveform = 0x80U;
    static const uint8_t ssd1680_partial_update_control_1[] = {0x00U, 0x80U};
    static const uint8_t ssd1683_partial_update_control_1[] = {0x00U, 0x00U};
    const uint8_t *const border_waveform = epaper->partial_refresh
        ? &partial_border_waveform
        : (is_ssd1683 ? &ssd1683_border_waveform : &ssd1680_border_waveform);
    /* SSD1683 monochrome full refresh bypasses the second RAM plane; colour
     * panels must use its actual red contents in both display modes. */
    const uint8_t *const update_control_1 = epaper->partial_refresh
        ? (is_ssd1683 ? ssd1683_partial_update_control_1 : ssd1680_partial_update_control_1)
        : (is_ssd1683
            ? (epaper->config.red_framebuffer != NULL ? ssd1683_partial_update_control_1
                                                     : ssd1683_update_control_1)
            : ssd1680_update_control_1);

    epaper->driver_output_data[0] = (uint8_t) height;
    epaper->driver_output_data[1] = (uint8_t) (height >> 8U);
    epaper->driver_output_data[2] = 0x00U;

    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = &command_driver_output_control,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[1U] = (SPI_SequenceStep) {
        .data = epaper->driver_output_data,
        .size = sizeof(epaper->driver_output_data),
        .prepare = dc_data,
    };
    epaper->steps[2U] = (SPI_SequenceStep) {
        .data = &command_data_entry_mode,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[3U] = (SPI_SequenceStep) {
        .data = &data_entry_mode,
        .size = 1U,
        .prepare = dc_data,
    };
    epaper->steps[4U] = (SPI_SequenceStep) {
        .data = &command_border_waveform_control,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[5U] = (SPI_SequenceStep) {
        .data = border_waveform,
        .size = sizeof(ssd1680_border_waveform),
        .prepare = dc_data,
    };
    epaper->steps[6U] = (SPI_SequenceStep) {
        .data = &command_display_update_control_1,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[7U] = (SPI_SequenceStep) {
        .data = update_control_1,
        .size = sizeof(ssd1680_update_control_1),
        .prepare = dc_data,
    };
    epaper->steps[8U] = (SPI_SequenceStep) {
        .data = &command_temperature_sensor_control,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[9U] = (SPI_SequenceStep) {
        .data = &temperature_sensor,
        .size = 1U,
        .prepare = dc_data,
    };
    epaper->steps[10U] = (SPI_SequenceStep) {
        .data = &command_display_update_control_2,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[11U] = (SPI_SequenceStep) {
        .data = &update_control_2,
        .size = 1U,
        .prepare = dc_data,
    };
    epaper->steps[12U] = (SPI_SequenceStep) {
        .data = &command_master_activation,
        .size = 1U,
        .prepare = dc_command,
    };
    /* Retained RAM needs no reset or full-mode LUT load between refreshes. */
    queue_or_error(fsm, epaper->baseline_valid ? 10U : 13U, EPAPER_STATE_WAIT_CONFIGURE);
}

static void epaper_wait_configure_service(FSM *fsm) {
    if (busy_is_ready(epaper_from_fsm(fsm))) {
        (void) FSM_Transition(fsm, EPAPER_STATE_WINDOW_SEND);
    }
}

static void epaper_window_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    const Epaper_Window *window = &epaper->refresh_window;
    const uint16_t end_y = window->y + window->height - 1U;
    epaper->ram_x_data[0] = (uint8_t) (window->x / 8U);
    epaper->ram_x_data[1] = (uint8_t) ((window->x + window->width - 1U) / 8U);
    epaper->ram_y_data[0] = (uint8_t) window->y;
    epaper->ram_y_data[1] = (uint8_t) (window->y >> 8U);
    epaper->ram_y_data[2] = (uint8_t) end_y;
    epaper->ram_y_data[3] = (uint8_t) (end_y >> 8U);
    epaper->transfer_row = window->y;

    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = &command_set_ram_x,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[1U] = (SPI_SequenceStep) {
        .data = epaper->ram_x_data,
        .size = sizeof(epaper->ram_x_data),
        .prepare = dc_data,
    };
    epaper->steps[2U] = (SPI_SequenceStep) {
        .data = &command_set_ram_y,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[3U] = (SPI_SequenceStep) {
        .data = epaper->ram_y_data,
        .size = sizeof(epaper->ram_y_data),
        .prepare = dc_data,
    };
    epaper->steps[4U] = (SPI_SequenceStep) {
        .data = &command_set_ram_x_counter,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[5U] = (SPI_SequenceStep) {
        .data = epaper->ram_x_data,
        .size = 1U,
        .prepare = dc_data,
    };
    epaper->steps[6U] = (SPI_SequenceStep) {
        .data = &command_set_ram_y_counter,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[7U] = (SPI_SequenceStep) {
        .data = epaper->ram_y_data,
        .size = 2U,
        .prepare = dc_data,
    };
    queue_or_error(fsm, 8U, EPAPER_STATE_BW_COMMAND_SEND);
}

static void epaper_bw_command_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = epaper->synchronizing ? &command_write_ram_red : &command_write_ram_bw,
        .size = 1U,
        .prepare = dc_command,
    };
    queue_or_error(fsm, 1U, EPAPER_STATE_BW_DATA_SEND);
}

/* Queue bounded batches of rows without a packed-region staging buffer. */
static void epaper_transfer_rows(FSM *fsm, const uint8_t *framebuffer,
                                 Epaper_State repeat_state, Epaper_State complete_state) {
    Epaper *epaper = epaper_from_fsm(fsm);
    const uint16_t end_row = epaper->refresh_window.y + epaper->refresh_window.height;
    const uint16_t start_byte = epaper->ram_x_data[0];
    const uint16_t row_bytes = epaper->ram_x_data[1] - start_byte + 1U;
    /* Full-width rows are contiguous and can share one DMA transfer. */
    const uint16_t rows_per_step = row_bytes == epaper->stride
        ? (uint16_t) (UINT16_MAX / row_bytes) : 1U;
    uint16_t count = 0U;
    while ((epaper->transfer_row < end_row) &&
           (count < sizeof(epaper->steps) / sizeof(epaper->steps[0]))) {
        const uint16_t remaining_rows = end_row - epaper->transfer_row;
        const uint16_t rows = remaining_rows < rows_per_step ? remaining_rows : rows_per_step;
        epaper->steps[count++] = (SPI_SequenceStep) {
            .data = framebuffer + (uint32_t) epaper->transfer_row * epaper->stride + start_byte,
            .size = (uint16_t) ((uint32_t) row_bytes * rows),
            .prepare = dc_data,
        };
        epaper->transfer_row += rows;
    }
    queue_or_error(fsm, count, epaper->transfer_row < end_row ? repeat_state : complete_state);
}

static void epaper_bw_data_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    const Epaper_State next = epaper->synchronizing ? EPAPER_STATE_COMPLETE
        : (epaper->partial_refresh ? EPAPER_STATE_UPDATE_SEND : EPAPER_STATE_RED_COMMAND_SEND);
    epaper_transfer_rows(fsm, epaper->config.black_framebuffer, EPAPER_STATE_BW_DATA_SEND, next);
}

static void epaper_red_command_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    /* Explicitly rewind the cursor before writing the second RAM plane. */
    epaper->transfer_row = epaper->refresh_window.y;
    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = &command_set_ram_x_counter, .size = 1U, .prepare = dc_command,
    };
    epaper->steps[1U] = (SPI_SequenceStep) {
        .data = epaper->ram_x_data, .size = 1U, .prepare = dc_data,
    };
    epaper->steps[2U] = (SPI_SequenceStep) {
        .data = &command_set_ram_y_counter, .size = 1U, .prepare = dc_command,
    };
    epaper->steps[3U] = (SPI_SequenceStep) {
        .data = epaper->ram_y_data, .size = 2U, .prepare = dc_data,
    };
    epaper->steps[4U] = (SPI_SequenceStep) {
        .data = &command_write_ram_red, .size = 1U, .prepare = dc_command,
    };
    queue_or_error(fsm, 5U, EPAPER_STATE_RED_DATA_SEND);
}

static void epaper_red_data_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    /* On monochrome panels RAM 0x26 is the previous-image reference. */
    const uint8_t *framebuffer = epaper->config.red_framebuffer != NULL
        ? epaper->config.red_framebuffer : epaper->config.black_framebuffer;
    epaper_transfer_rows(fsm, framebuffer, EPAPER_STATE_RED_DATA_SEND, EPAPER_STATE_UPDATE_SEND);
}

static void epaper_update_send_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    static const uint8_t full_update_control = 0xF7U;
    static const uint8_t partial_update_control = 0xFFU;
    epaper->steps[0U] = (SPI_SequenceStep) {
        .data = &command_display_update_control_2,
        .size = 1U,
        .prepare = dc_command,
    };
    epaper->steps[1U] = (SPI_SequenceStep) {
        .data = epaper->partial_refresh ? &partial_update_control : &full_update_control,
        .size = 1U,
        .prepare = dc_data,
    };
    epaper->steps[2U] = (SPI_SequenceStep) {
        .data = &command_master_activation,
        .size = 1U,
        .prepare = dc_command,
    };
    queue_or_error(fsm, 3U, EPAPER_STATE_WAIT_UPDATE);
}

static void epaper_wait_update_service(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    if (busy_is_ready(epaper)) {
        /* Synchronize the old-image RAM only after the displayed image settles.
         * On colour panels this RAM holds red and must remain untouched. */
        epaper->synchronizing = epaper->config.red_framebuffer == NULL;
        (void) FSM_Transition(fsm, epaper->synchronizing
                                  ? EPAPER_STATE_WINDOW_SEND : EPAPER_STATE_COMPLETE);
    }
}

static void epaper_complete_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    /* F7/FF disable analog power and the oscillator, retaining accessible RAM. */
    epaper->refresh_forced_full = false;
    epaper->baseline_valid = true;
    epaper->dirty = false;
    epaper->red_dirty = false;
    epaper->dirty_window = (Epaper_Window) {0};
    epaper->synchronizing = false;
    (void) FSM_Transition(fsm, EPAPER_STATE_IDLE);
}

static void epaper_error_enter(FSM *fsm) {
    Epaper *epaper = epaper_from_fsm(fsm);
    epaper->force_next_refresh_full |= epaper->refresh_forced_full;
    epaper->refresh_forced_full = false;
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
        (config->busy.port == NULL) || (config->busy.pin == 0U) ||
        (config->controller > EPAPER_CONTROLLER_SSD1683)) {
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
    const bool force_full = epaper->force_next_refresh_full;
    if (!force_full && epaper->baseline_valid && !epaper->dirty) {
        return true;
    }

    const uint32_t dirty_area = (uint32_t) epaper->dirty_window.width * epaper->dirty_window.height;
    const uint32_t visible_area = (uint32_t) epaper->config.window.width * epaper->config.window.height;
    epaper->partial_refresh = !force_full && epaper->baseline_valid && !epaper->red_dirty &&
                             ((uint64_t) dirty_area * 2U < visible_area);
    epaper->synchronizing = false;
    if (epaper->partial_refresh) {
        epaper->refresh_window = epaper->dirty_window;
        const uint16_t start_x = epaper->dirty_window.x & (uint16_t) ~7U;
        const uint32_t end_x = ((uint32_t) epaper->dirty_window.x +
                               epaper->dirty_window.width + 7U) & ~7UL;
        epaper->refresh_window.x = start_x;
        epaper->refresh_window.width = (uint16_t) (
            (end_x > epaper->config.width ? epaper->config.width : end_x) - start_x);
    } else {
        epaper->refresh_window = (Epaper_Window) {
            .width = epaper->config.width, .height = epaper->config.height,
        };
    }
    if (!FSM_Transition(&epaper->fsm, epaper->baseline_valid
                          ? EPAPER_STATE_CONFIGURE_SEND : EPAPER_STATE_RESET_ASSERT)) {
        return false;
    }
    /* Keep requests made during this transfer pending for the following one. */
    epaper->refresh_forced_full = force_full;
    epaper->force_next_refresh_full = false;
    return true;
}

void Epaper_ForceNextRefreshFull(Epaper *epaper) {
    if (epaper != NULL) {
        epaper->force_next_refresh_full = true;
    }
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

static void epaper_track_pixel(Epaper *epaper, uint16_t x, uint16_t y,
                                uint32_t offset, uint8_t old_black, uint8_t old_red) {
    const bool red_changed = epaper->config.red_framebuffer != NULL &&
                            old_red != epaper->config.red_framebuffer[offset];
    if ((old_black == epaper->config.black_framebuffer[offset]) && !red_changed) {
        return;
    }
    epaper->red_dirty |= red_changed;
    if (!epaper->dirty) {
        epaper->dirty_window = (Epaper_Window) {x, y, 1U, 1U};
        epaper->dirty = true;
        return;
    }
    Epaper_Window *bounds = &epaper->dirty_window;
    const uint16_t end_x = bounds->x + bounds->width;
    const uint16_t end_y = bounds->y + bounds->height;
    if (x < bounds->x) {
        bounds->x = x;
    }
    if (y < bounds->y) {
        bounds->y = y;
    }
    bounds->width = (x >= end_x ? x + 1U : end_x) - bounds->x;
    bounds->height = (y >= end_y ? y + 1U : end_y) - bounds->y;
}

void Epaper_SetPixel(Epaper *epaper, uint16_t x, uint16_t y, uint8_t colour) {
    uint16_t physical_x;
    uint16_t physical_y;
    if ((epaper == NULL) || (colour > EPAPER_COLOUR_RED) ||
        !map_pixel(epaper, x, y, &physical_x, &physical_y) ||
        ((colour == EPAPER_COLOUR_RED) && (epaper->config.red_framebuffer == NULL))) {
        return;
    }

    const uint32_t offset = ((uint32_t) physical_y * epaper->stride) + (physical_x / 8U);
    const uint8_t bit = (uint8_t) (0x80U >> (physical_x & 7U));
    const uint8_t old_black = epaper->config.black_framebuffer[offset];
    const uint8_t old_red = epaper->config.red_framebuffer != NULL
        ? epaper->config.red_framebuffer[offset] : 0U;
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
    epaper_track_pixel(epaper, physical_x, physical_y, offset, old_black, old_red);
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
    const uint8_t old_black = epaper->config.black_framebuffer[offset];
    const uint8_t old_red = epaper->config.red_framebuffer != NULL
        ? epaper->config.red_framebuffer[offset] : 0U;
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
    epaper_track_pixel(epaper, physical_x, physical_y, offset, old_black, old_red);
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
