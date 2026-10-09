#include "mode/puzzle/underground/underground.h"
#include "fonts/glyph.h"
#include "fonts/dot-matrix-bold.h"
#include "fonts/london-underground.h"
#include "fonts/render.h"
#include "mode.h"
#include "pwm/pwm.h"
#include "sys/gpio.h"
#include "sys/rng.h"
#include "sys/rtc.h"
#include "sys/tick.h"
#include "stm32h5xx_it.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DOOR_BUTTON_Pin GPIO_B6_Pin
#define DOOR_BUTTON_Port GPIO_B6_Port
#define DISPLAY_CS_Pin GPIO_C7_Pin
#define DISPLAY_CS_Port GPIO_C7_Port
#define DISPLAY_DC_Pin GPIO_C6_Pin
#define DISPLAY_DC_Port GPIO_C6_Port
#define DISPLAY_RESET_Pin GPIO_C5_Pin
#define DISPLAY_RESET_Port GPIO_C5_Port

#define UNDERGROUND_STARTUP_DURATION_MS 3000U
#define UNDERGROUND_STARTUP_LAMP_HALF_PERIOD_MS 250U
#define UNDERGROUND_LAMP_ON_DUTY_PERCENT 2U

/* Text top for zero-based lines 0..4: 2, 15, 28, 41, 54.
 * A 2 px top margin with 13 px between lines; the time stays bottom-aligned. */
#define DISPLAY_Y(line) (2U + (line) * 13U)
#define DISPLAY_X_MARGIN 0
#define UNDERGROUND_DEPARTURE_COUNT 3U
#define UNDERGROUND_GAME_MINUTE_SECONDS 3U
#define UNDERGROUND_TRAIN_MOVE_PERIOD_SECONDS 2U
#define UNDERGROUND_FORCIBLY_DISEMBARKED_DURATION_SECONDS 3U
#define UNDERGROUND_ATTRACT_IDLE_MS 10000U
#define UNDERGROUND_ATTRACT_MIN_STOPS 3U
#define UNDERGROUND_ATTRACT_MAX_STOPS 6U

typedef struct {
    UndergroundRouteID route_id;
    uint8_t station_stop_index;
    uint32_t arrival_seconds;
    uint8_t minutes_until;
} Underground_Departure;

static Underground_Data *const underground = &mode_data.mode.underground;
static const GPIO_PinDef underground_lamp_pin = {GPIO_B7_Port, GPIO_B7_Pin};
static const GPIO_PinDef underground_button_pin = {
    DOOR_BUTTON_Port, DOOR_BUTTON_Pin,
};
static const font_t underground_london_underground_font = {
    .glyphs = london_underground_glyphs,
    .glyph_count = london_underground_glyph_count,
    .character_map = london_underground_ascii_map,
    .character_map_count = sizeof(london_underground_ascii_map) /
                            sizeof(london_underground_ascii_map[0]),
    .bitmap = london_underground_bitmap,
    .line_height = london_underground_line_height,
    .ascent = london_underground_ascent,
};
static const font_t underground_dot_matrix_font = {
    .glyphs = dot_matrix_glyphs,
    .glyph_count = dot_matrix_glyph_count,
    .character_map = dot_matrix_ascii_map,
    .character_map_count = sizeof(dot_matrix_ascii_map) / sizeof(dot_matrix_ascii_map[0]),
    .bitmap = dot_matrix_bitmap,
    .line_height = dot_matrix_line_height,
    .ascent = dot_matrix_ascent,
};

static void underground_game_at_station_service(FSM *fsm);
static void underground_game_on_train_enter(FSM *fsm);
static void underground_game_on_train_service(FSM *fsm);
static void underground_game_forcibly_disembarked_enter(FSM *fsm);
static void underground_game_forcibly_disembarked_service(FSM *fsm);

static const FSM_State underground_game_fsm_states[UNDERGROUND_GAME_FSM_STATE_COUNT] = {
    [UNDERGROUND_GAME_FSM_STATE_AT_STATION] = {
        .service = underground_game_at_station_service,
        .next_mask = FSM_NEXT(UNDERGROUND_GAME_FSM_STATE_ON_TRAIN),
    },
    [UNDERGROUND_GAME_FSM_STATE_ON_TRAIN] = {
        .enter = underground_game_on_train_enter,
        .service = underground_game_on_train_service,
        .next_mask = FSM_NEXT(UNDERGROUND_GAME_FSM_STATE_AT_STATION) |
                     FSM_NEXT(UNDERGROUND_GAME_FSM_STATE_FORCIBLY_DISEMBARKED),
    },
    [UNDERGROUND_GAME_FSM_STATE_FORCIBLY_DISEMBARKED] = {
        .enter = underground_game_forcibly_disembarked_enter,
        .service = underground_game_forcibly_disembarked_service,
        .next_mask = FSM_NEXT(UNDERGROUND_GAME_FSM_STATE_AT_STATION),
    },
};

static void underground_set_pixel(void *context, uint16_t x, uint16_t y, uint8_t colour) {
    SSD1322_SetPixel((SSD1322 *) context, x, y, colour);
}

static bool underground_lamp_init(void) {
    return PWM_Setup(&underground_lamp_pin);
}

static void underground_lamp_set(const bool enabled) {
    PWM_SetDuty(&underground_lamp_pin, enabled ? UNDERGROUND_LAMP_ON_DUTY_PERCENT : 0U);
}

static uint8_t underground_route_station_stop_index(const UndergroundRoute *route,
                                                    const UndergroundStationID station_id) {
    for (uint8_t stop_index = 0U; stop_index < route->stops_count; stop_index++) {
        if (underground_route_stops[route->stops_offset + stop_index] == station_id) {
            return stop_index;
        }
    }
    return UNDERGROUND_NO_VIA_STOP_INDEX;
}

static void underground_insert_departure(Underground_Departure departures[UNDERGROUND_DEPARTURE_COUNT],
                                         uint8_t *departure_count,
                                         const Underground_Departure departure) {
    uint8_t position = 0U;
    while (position < *departure_count &&
           (departures[position].arrival_seconds < departure.arrival_seconds ||
            (departures[position].arrival_seconds == departure.arrival_seconds &&
             departures[position].route_id < departure.route_id))) {
        position++;
    }
    if (position == UNDERGROUND_DEPARTURE_COUNT) {
        return;
    }

    const uint8_t last = *departure_count < UNDERGROUND_DEPARTURE_COUNT
                             ? *departure_count : UNDERGROUND_DEPARTURE_COUNT - 1U;
    for (uint8_t index = last; index > position; index--) {
        departures[index] = departures[index - 1U];
    }
    departures[position] = departure;
    if (*departure_count < UNDERGROUND_DEPARTURE_COUNT) {
        (*departure_count)++;
    }
}

static uint8_t underground_next_departures(Underground_Game *game,
                                           Underground_Departure departures[UNDERGROUND_DEPARTURE_COUNT]) {
    if (game->station >= UNDERGROUND_STATION_COUNT) {
        return 0U;
    }

    uint8_t departure_count = 0U;
    const UndergroundStation *const station = &underground_stations[game->station];
    UndergroundRouteID next_route = UNDERGROUND_ROUTE_COUNT;
    uint32_t next_arrival_seconds = UINT32_MAX;

    game->arriving_route = UNDERGROUND_NO_STATION;
    for (uint8_t station_route_index = 0U;
         station_route_index < station->routes_count;
         station_route_index++) {
        const UndergroundRouteID route_id =
            underground_station_routes[station->routes_offset + station_route_index];
        if (underground_routes[route_id].destination == game->station) {
            continue;
        }
        const uint32_t arrival_seconds = game->route_next_arrival_seconds[route_id];
        if (arrival_seconds <= game->seconds &&
            (arrival_seconds < next_arrival_seconds ||
             (arrival_seconds == next_arrival_seconds && route_id < next_route))) {
            next_route = route_id;
            next_arrival_seconds = arrival_seconds;
        }
    }

    if (next_route < UNDERGROUND_ROUTE_COUNT &&
        (game->last_arrival_seconds == UINT32_MAX ||
         game->seconds - game->last_arrival_seconds > 1U)) {
        const UndergroundRoute *const route = &underground_routes[next_route];
        game->arriving_route = next_route;
        game->last_arrival_seconds = game->seconds;
        /* A held train establishes a new headway for this route.  Retaining
         * the original timetable here would make its following services catch
         * up and appear as a compressed group on the board. */
        game->route_next_arrival_seconds[next_route] =
            game->seconds + route->every_minutes * UNDERGROUND_GAME_MINUTE_SECONDS;
        underground_insert_departure(departures, &departure_count,
                                     (Underground_Departure) {
                                         .route_id = next_route,
                                         .station_stop_index =
                                             underground_route_station_stop_index(route, game->station),
                                         .arrival_seconds = next_arrival_seconds,
                                         .minutes_until = 0U,
                                     });
    }

    for (uint8_t station_route_index = 0U;
         station_route_index < station->routes_count;
         station_route_index++) {
        const UndergroundRouteID route_id =
            underground_station_routes[station->routes_offset + station_route_index];
        const UndergroundRoute *const route = &underground_routes[route_id];
        if (route->destination == game->station) {
            continue;
        }
        const uint32_t arrival_seconds = game->route_next_arrival_seconds[route_id];
        const uint32_t interval_seconds = route->every_minutes * UNDERGROUND_GAME_MINUTE_SECONDS;
        const uint8_t stop_index = underground_route_station_stop_index(route, game->station);

        for (uint8_t occurrence = 0U; occurrence < UNDERGROUND_DEPARTURE_COUNT; occurrence++) {
            const uint32_t candidate_seconds = arrival_seconds + occurrence * interval_seconds;
            const uint32_t seconds_until = candidate_seconds <= game->seconds
                                               ? 1U : candidate_seconds - game->seconds;
            underground_insert_departure(departures, &departure_count,
                                         (Underground_Departure) {
                                             .route_id = route_id,
                                             .station_stop_index = stop_index,
                                             .arrival_seconds = candidate_seconds,
                                             .minutes_until =
                                                 (uint8_t) ((seconds_until +
                                                             UNDERGROUND_GAME_MINUTE_SECONDS - 1U) /
                                                            UNDERGROUND_GAME_MINUTE_SECONDS),
                                         });
        }
    }
    return departure_count;
}

static void underground_at_station_display(SSD1322 *display,
                                           const UndergroundStationID station_id,
                                           const Underground_Departure departures[UNDERGROUND_DEPARTURE_COUNT],
                                           const uint8_t departure_count) {
    const char *const station_name = station_id < UNDERGROUND_STATION_COUNT
                                         ? underground_stations[station_id].name
                                         : "Unknown station";
    static char heading[64];
    const int heading_length = snprintf(heading, sizeof(heading), "-- %s --", station_name);
    RTC_Time time;
    RTC_GetTime(&time);
    static char time_text[9];
    const int time_length = snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u",
                                     time.hours, time.minutes, time.seconds);

    Font_Text(underground_set_pixel, display, (const uint8_t *) heading, (uint16_t) heading_length,
              &underground_london_underground_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(0),
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             london_underground_line_height},
              FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);

    const uint16_t platform_width = 7U;
    const uint16_t column_gap = 5U;
    const uint16_t route_x = DISPLAY_X_MARGIN + platform_width + column_gap;
    const uint16_t mins_width = 22U;
    const uint16_t mins_x = SSD1322_Width(display) - DISPLAY_X_MARGIN - mins_width;
    const uint16_t minutes_width = 14U;
    const uint16_t minutes_x = mins_x - column_gap - minutes_width;
    for (uint8_t index = 0U; index < departure_count; index++) {
        const Underground_Departure *const departure = &departures[index];
        const UndergroundRoute *const route = &underground_routes[departure->route_id];
        const char *const destination_name = underground_stations[route->destination].name;
        const bool via_applies = route->via != UNDERGROUND_NO_STATION &&
                                 departure->station_stop_index < route->via_stop_index;
        const char *const via_name = via_applies ? underground_stations[route->via].name : "";
        static char route_text[128];
        const int route_length = snprintf(route_text, sizeof(route_text),
                                          via_applies ? "%s via %s" : "%s",
                                          destination_name, via_name);
        static char order[2];
        static char minutes[4];
        order[0] = (char) ('1' + index);
        order[1] = '\0';
        const int minutes_length = snprintf(minutes, sizeof(minutes), "%u", departure->minutes_until);
        const char *const minute_label = departure->minutes_until == 1U ? "min" : "mins";
        const uint16_t y = DISPLAY_Y(index + 1U);

        Font_Text(underground_set_pixel, display, (const uint8_t *) order, 1U,
                  &underground_london_underground_font,
                  (Font_Bounds) {DISPLAY_X_MARGIN, y, platform_width, london_underground_line_height},
                  FONT_ALIGN_RIGHT, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
        Font_Text(underground_set_pixel, display, (const uint8_t *) route_text, (uint16_t) route_length,
                  &underground_london_underground_font,
                  (Font_Bounds) {route_x, y, minutes_x - column_gap - route_x,
                                 london_underground_line_height},
                  FONT_ALIGN_LEFT, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
        if (departure->minutes_until != 0U) {
            Font_Text(underground_set_pixel, display, (const uint8_t *) minutes,
                      (uint16_t) minutes_length, &underground_london_underground_font,
                      (Font_Bounds) {minutes_x, y, minutes_width, london_underground_line_height},
                      FONT_ALIGN_RIGHT, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
            Font_Text(underground_set_pixel, display, (const uint8_t *) minute_label,
                      strlen(minute_label), &underground_london_underground_font,
                      (Font_Bounds) {mins_x, y, mins_width, london_underground_line_height},
                      FONT_ALIGN_LEFT, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
        }
    }

    /* Digits have no descenders: place the font's unused descent below the display. */
    Font_Text(underground_set_pixel, display, (const uint8_t *) time_text, (uint16_t) time_length,
              &underground_dot_matrix_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(4),
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             SSD1322_Height(display) - DISPLAY_Y(4) + dot_matrix_descent},
              FONT_ALIGN_CENTRE, FONT_ALIGN_BOTTOM, SSD1322_COLOUR_WHITE);
}

static void underground_on_train_display(SSD1322 *display,
                                         const UndergroundRouteID route_id,
                                         const uint8_t stop_index,
                                         const bool at_station) {
    const UndergroundRoute *const route = &underground_routes[route_id];
    const char *const line_name = underground_lines[route->line].name;
    const char *const destination_name = underground_stations[route->destination].name;
    static char train_text[96];
    static char station_text[64];
    static const uint8_t disembark_text[] = "Disembark here for";
    const int train_length = snprintf(train_text, sizeof(train_text), "%s Line to %s",
                                      line_name, destination_name);
    const int station_length = at_station
        ? snprintf(station_text, sizeof(station_text), "** %s **",
                   underground_stations[underground_route_stops[route->stops_offset + stop_index]].name)
        : snprintf(station_text, sizeof(station_text), "In Transit");
    RTC_Time time;
    RTC_GetTime(&time);
    static char time_text[9];
    const int time_length = snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u",
                                     time.hours, time.minutes, time.seconds);

    Font_Text(underground_set_pixel, display, (const uint8_t *) train_text, (uint16_t) train_length,
              &underground_london_underground_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(0),
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             london_underground_line_height},
              FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
    Font_Text(underground_set_pixel, display,
              at_station ? disembark_text : (const uint8_t *) station_text,
              at_station ? sizeof(disembark_text) - 1U : (uint16_t) station_length,
              &underground_london_underground_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(1),
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             london_underground_line_height},
              FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
    if (at_station) {
        Font_Text(underground_set_pixel, display, (const uint8_t *) station_text,
                  (uint16_t) station_length, &underground_london_underground_font,
                  (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(2),
                                 SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                                 london_underground_line_height},
                  FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
    }
    Font_Text(underground_set_pixel, display, (const uint8_t *) time_text, (uint16_t) time_length,
              &underground_dot_matrix_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, DISPLAY_Y(4),
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             SSD1322_Height(display) - DISPLAY_Y(4) + dot_matrix_descent},
              FONT_ALIGN_CENTRE, FONT_ALIGN_BOTTOM, SSD1322_COLOUR_WHITE);
}

static void underground_forcibly_disembarked_display(SSD1322 *display,
                                                      const bool out_of_zone) {
    static const uint8_t heading[] = "Forcibly Disembarked";
    static const uint8_t out_of_zone_text[] = "Out of Zone 1 & 2";
    static const uint8_t end_of_line_text[] = "End of Line";
    const uint8_t *const detail = out_of_zone ? out_of_zone_text : end_of_line_text;
    const uint16_t detail_length = out_of_zone ? sizeof(out_of_zone_text) - 1U
                                                : sizeof(end_of_line_text) - 1U;
    const uint16_t text_top =
        (SSD1322_Height(display) - 2U * london_underground_line_height) / 2U;

    Font_Text(underground_set_pixel, display, heading, sizeof(heading) - 1U,
              &underground_london_underground_font,
              (Font_Bounds) {DISPLAY_X_MARGIN, text_top,
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             london_underground_line_height},
              FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
    Font_Text(underground_set_pixel, display, detail, detail_length,
              &underground_london_underground_font,
              (Font_Bounds) {DISPLAY_X_MARGIN,
                             text_top + london_underground_line_height,
                             SSD1322_Width(display) - 2U * DISPLAY_X_MARGIN,
                             london_underground_line_height},
              FONT_ALIGN_CENTRE, FONT_ALIGN_TOP, SSD1322_COLOUR_WHITE);
}

static void underground_game_at_station_service(FSM *fsm) {
    Underground_Game *const game = fsm->context;
    if (game == NULL) {
        return;
    }

    if (game->arriving_route != UNDERGROUND_NO_STATION && underground->button_pressed) {
        underground->button_pressed = false;
        game->route = game->arriving_route;
        game->route_stop_index =
            underground_route_station_stop_index(&underground_routes[game->route], game->station);
        game->train_at_station = true;
        game->train_next_move_seconds = game->seconds + UNDERGROUND_TRAIN_MOVE_PERIOD_SECONDS;
        FSM_Transition(fsm, UNDERGROUND_GAME_FSM_STATE_ON_TRAIN);
        return;
    }
    if (!tick_1hz || !SSD1322_IsReady(&underground->display)) {
        return;
    }

    game->seconds++;
    Underground_Departure departures[UNDERGROUND_DEPARTURE_COUNT];
    const uint8_t departure_count = underground_next_departures(game, departures);

    SSD1322_Fill(&underground->display, 0, 0,
                 SSD1322_Width(&underground->display), SSD1322_Height(&underground->display),
                 SSD1322_COLOUR_BLACK);
    underground_at_station_display(&underground->display, game->station,
                                   departures, departure_count);
    SSD1322_Refresh(&underground->display);
    underground_lamp_set(game->arriving_route != UNDERGROUND_NO_STATION);
}

static void underground_game_on_train_enter(FSM *fsm) {
    const Underground_Game *const game = fsm->context;
    if (game == NULL || !SSD1322_IsReady(&underground->display)) {
        return;
    }

    SSD1322_Fill(&underground->display, 0, 0,
                 SSD1322_Width(&underground->display), SSD1322_Height(&underground->display),
                 SSD1322_COLOUR_BLACK);
    underground_on_train_display(&underground->display, game->route, game->route_stop_index,
                                 game->train_at_station);
    SSD1322_Refresh(&underground->display);
    underground_lamp_set(game->train_at_station);
}

static void underground_game_on_train_service(FSM *fsm) {
    Underground_Game *const game = fsm->context;
    if (game == NULL) {
        return;
    }

    if (game->train_at_station && underground->button_pressed) {
        underground->button_pressed = false;
        game->station = underground_route_stops[
            underground_routes[game->route].stops_offset + game->route_stop_index];
        game->arriving_route = UNDERGROUND_NO_STATION;
        underground_lamp_set(false);
        FSM_Transition(fsm, UNDERGROUND_GAME_FSM_STATE_AT_STATION);
        return;
    }
    if (!tick_1hz || !SSD1322_IsReady(&underground->display)) {
        return;
    }

    game->seconds++;
    if (game->train_at_station) {
        const UndergroundRoute *const route = &underground_routes[game->route];
        const UndergroundStationID current_station =
            underground_route_stops[route->stops_offset + game->route_stop_index];
        const uint8_t next_stop_index = game->route_stop_index + 1U;
        if (current_station == route->destination || next_stop_index >= route->stops_count ||
            underground_stations[underground_route_stops[route->stops_offset + next_stop_index]].in_zone == 0U) {
            /* TODO: Record a strike when a passenger is ejected from the zone. */
            game->station = current_station;
            game->ejected_out_of_zone = current_station != route->destination &&
                                        next_stop_index < route->stops_count;
            game->ejected_at_seconds = game->seconds;
            underground_lamp_set(false);
            FSM_Transition(fsm, UNDERGROUND_GAME_FSM_STATE_FORCIBLY_DISEMBARKED);
            return;
        }
        game->train_at_station = false;
    } else if (game->seconds >= game->train_next_move_seconds) {
        const uint8_t next_stop_index = game->route_stop_index + 1U;
        game->route_stop_index = next_stop_index;
        game->train_at_station = true;
        game->train_next_move_seconds = game->seconds + UNDERGROUND_TRAIN_MOVE_PERIOD_SECONDS;
    }

    SSD1322_Fill(&underground->display, 0, 0,
                 SSD1322_Width(&underground->display), SSD1322_Height(&underground->display),
                 SSD1322_COLOUR_BLACK);
    underground_on_train_display(&underground->display, game->route, game->route_stop_index,
                                 game->train_at_station);
    SSD1322_Refresh(&underground->display);
    underground_lamp_set(game->train_at_station);
}

static void underground_game_forcibly_disembarked_enter(FSM *fsm) {
    Underground_Game *const game = fsm->context;
    underground_lamp_set(false);
    if (game == NULL) {
        return;
    }
    game->ejection_started = true;
    if (!SSD1322_IsReady(&underground->display)) {
        return;
    }

    SSD1322_Fill(&underground->display, 0, 0,
                 SSD1322_Width(&underground->display), SSD1322_Height(&underground->display),
                 SSD1322_COLOUR_BLACK);
    underground_forcibly_disembarked_display(&underground->display, game->ejected_out_of_zone);
    SSD1322_Refresh(&underground->display);
}

static void underground_game_forcibly_disembarked_service(FSM *fsm) {
    Underground_Game *const game = fsm->context;
    if (game == NULL || !tick_1hz) {
        return;
    }

    if (game->ejection_started) {
        game->ejection_started = false;
        return;
    }

    game->seconds++;
    if (game->seconds - game->ejected_at_seconds >= UNDERGROUND_FORCIBLY_DISEMBARKED_DURATION_SECONDS) {
        FSM_Transition(fsm, UNDERGROUND_GAME_FSM_STATE_AT_STATION);
    }
}

static void underground_init_enter(FSM *fsm) {
    (void) fsm;
    if (!underground_lamp_init()) {
        Error_Handler();
    }
    underground_lamp_set(false);

    underground->button_queue = (IM_EventQueue){0};
    underground->button_pressed = false;
    underground->button_channel_state[0] = (IM_DigitalChannelState){0};
    underground->button_input_state = (IM_DigitalInputState){
        .channels = underground->button_channel_state,
    };
    underground->button_input_config = (IM_DigitalInputConfig){
        .rows = &underground_button_pin,
        .row_count = 1,
        .queue = &underground->button_queue,
        .event_mask = IM_EVENT_DOWN,
        .state = &underground->button_input_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .pull = IM_DIGITAL_INPUT_PULL_NONE,
        .active_high = false,
    };
    underground->button_handle = IM_RegisterDigital(&underground->button_input_config);

    const SSD1322_Config display_config = {
        .framebuffer = underground->framebuffer,
        .framebuffer_size = sizeof(underground->framebuffer),
        .baud = SPI_BAUD_8MHZ,
        .cs_port = DISPLAY_CS_Port,
        .cs_pin = DISPLAY_CS_Pin,
        .dc = {DISPLAY_DC_Port, DISPLAY_DC_Pin},
        .reset = {DISPLAY_RESET_Port, DISPLAY_RESET_Pin},
        .column_offset = 0x1c,
        .row_offset = 0,
        .dual_com = true,
        .com_split = false,
        .com_reverse = true,
    };
    underground->display_init_failed = !SSD1322_Init(&underground->display, &display_config);
    Mode_SetServiceEnabled(true);
}

static void underground_always_service(void) {
    underground->button_pressed = false;
    IM_Event event;
    while (IM_EventQueue_Read(&underground->button_queue, &event)) {
        underground->button_pressed |= event.event == IM_EVENT_DOWN;
    }

    if (!underground->display_init_failed) {
        SSD1322_Service(&underground->display);
    }
}

static void underground_init_service(FSM *fsm) {
    if (!underground->display_init_failed && SSD1322_IsReady(&underground->display)) {
        FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
    }
}

static void underground_startup_enter(FSM *fsm) {
    (void) fsm;
    underground->startup_clear_started = false;
    SSD1322_DrawTestCard(&underground->display);
    SSD1322_Refresh(&underground->display);
    underground->startup_started_ms = HAL_GetTick();
    underground_lamp_set(true);
}

static void underground_startup_service(FSM *fsm) {
    const uint32_t elapsed_ms = HAL_GetTick() - underground->startup_started_ms;
    if (elapsed_ms < UNDERGROUND_STARTUP_DURATION_MS) {
        underground_lamp_set((elapsed_ms / UNDERGROUND_STARTUP_LAMP_HALF_PERIOD_MS) % 2U == 0U);
        return;
    }

    underground_lamp_set(false);
    if (!SSD1322_IsReady(&underground->display)) {
        return;
    }

    if (!underground->startup_clear_started) {
        SSD1322_Fill(&underground->display, 0, 0,
                     SSD1322_Width(&underground->display), SSD1322_Height(&underground->display),
                     SSD1322_COLOUR_BLACK);
        underground->startup_clear_started = SSD1322_Refresh(&underground->display);
        return;
    }
    FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
}

static void underground_attract_enter(FSM *fsm) {
    (void) fsm;
    IM_EventQueue_Clear(&underground->button_queue);
    underground->button_pressed = false;
    underground->attract_last_input_ms = HAL_GetTick();
    underground->attract_route = UNDERGROUND_NO_STATION;
    underground->attract_disembark_stop_index = UNDERGROUND_NO_VIA_STOP_INDEX;
    do {
        underground->game.station =
            (UndergroundStationID) TRNG_Rand32Range(0U, UNDERGROUND_STATION_COUNT - 1U);
    } while (underground_stations[underground->game.station].in_zone == 0U);
    underground->game.seconds = 0U;
    underground->game.arriving_route = UNDERGROUND_NO_STATION;
    underground->game.last_arrival_seconds = UINT32_MAX;
    underground->game.route = UNDERGROUND_NO_STATION;
    underground->game.route_stop_index = 0U;
    underground->game.train_at_station = false;
    underground->game.train_next_move_seconds = 0U;
    underground->game.ejected_out_of_zone = false;
    underground->game.ejected_at_seconds = 0U;
    underground->game.ejection_started = false;
    for (UndergroundRouteID route_id = 0U; route_id < UNDERGROUND_ROUTE_COUNT; route_id++) {
        const uint8_t interval = underground_routes[route_id].every_minutes;
        underground->game.route_next_arrival_seconds[route_id] =
            (route_id % interval + 1U) * UNDERGROUND_GAME_MINUTE_SECONDS;
    }
    FSM_Init(&underground->game.fsm, underground_game_fsm_states,
             UNDERGROUND_GAME_FSM_STATE_AT_STATION, &underground->game);
}

/* Stop before a terminus or the first stop outside zones 1 and 2. */
static uint8_t underground_attract_last_stop(const UndergroundRoute *route,
                                            const uint8_t start_stop_index) {
    uint8_t last_stop_index = start_stop_index;
    while (last_stop_index + 1U < route->stops_count &&
           underground_route_stops[route->stops_offset + last_stop_index] != route->destination &&
           underground_stations[underground_route_stops[
               route->stops_offset + last_stop_index + 1U]].in_zone != 0U) {
        last_stop_index++;
    }
    return last_stop_index;
}

static void underground_attract_choose_route(Underground_Game *game) {
    const UndergroundStation *const station = &underground_stations[game->station];
    UndergroundRouteID routes[UNDERGROUND_ROUTE_COUNT];
    UndergroundLineID lines[UNDERGROUND_LINE_COUNT];
    uint8_t route_count = 0U;
    uint8_t line_count = 0U;
    for (uint8_t index = 0U; index < station->routes_count; index++) {
        const UndergroundRouteID route_id = underground_station_routes[station->routes_offset + index];
        const UndergroundRoute *const route = &underground_routes[route_id];
        const uint8_t stop_index = underground_route_station_stop_index(route, game->station);
        if (stop_index == UNDERGROUND_NO_VIA_STOP_INDEX ||
            underground_attract_last_stop(route, stop_index) == stop_index) {
            continue;
        }
        routes[route_count++] = route_id;
        uint8_t line_index = 0U;
        while (line_index < line_count && lines[line_index] != route->line) {
            line_index++;
        }
        if (line_index == line_count) {
            lines[line_count++] = route->line;
        }
    }
    if (route_count == 0U) {
        return;
    }

    /* Choose the line first so lines with more route variants are not favoured. */
    const UndergroundLineID line = lines[TRNG_Rand32Range(0U, line_count - 1U)];
    uint8_t matching_count = 0U;
    for (uint8_t index = 0U; index < route_count; index++) {
        if (underground_routes[routes[index]].line == line) {
            routes[matching_count++] = routes[index];
        }
    }
    underground->attract_route = routes[TRNG_Rand32Range(0U, matching_count - 1U)];
}

static void underground_attract_service(FSM *fsm) {
    (void) fsm;
    Underground_Game *const game = &underground->game;
    if (underground->button_pressed) {
        underground->attract_last_input_ms = HAL_GetTick();
        underground->attract_route = UNDERGROUND_NO_STATION;
        underground->attract_disembark_stop_index = UNDERGROUND_NO_VIA_STOP_INDEX;
    } else if (HAL_GetTick() - underground->attract_last_input_ms >= UNDERGROUND_ATTRACT_IDLE_MS) {
        if (game->fsm.current_id == UNDERGROUND_GAME_FSM_STATE_AT_STATION) {
            underground->attract_disembark_stop_index = UNDERGROUND_NO_VIA_STOP_INDEX;
            if (underground->attract_route == UNDERGROUND_NO_STATION) {
                underground_attract_choose_route(game);
            }
            underground->button_pressed = underground->attract_route != UNDERGROUND_NO_STATION &&
                                          game->arriving_route == underground->attract_route;
        } else if (game->fsm.current_id == UNDERGROUND_GAME_FSM_STATE_ON_TRAIN) {
            if (underground->attract_disembark_stop_index == UNDERGROUND_NO_VIA_STOP_INDEX) {
                const UndergroundRoute *const route = &underground_routes[game->route];
                const uint8_t available_stops =
                    underground_attract_last_stop(route, game->route_stop_index) - game->route_stop_index;
                const uint8_t max_stops = available_stops < UNDERGROUND_ATTRACT_MAX_STOPS
                                             ? available_stops : UNDERGROUND_ATTRACT_MAX_STOPS;
                const uint8_t min_stops = max_stops < UNDERGROUND_ATTRACT_MIN_STOPS
                                             ? max_stops : UNDERGROUND_ATTRACT_MIN_STOPS;
                underground->attract_disembark_stop_index = game->route_stop_index +
                    TRNG_Rand32Range(min_stops, max_stops);
            }
            if (game->train_at_station &&
                game->route_stop_index >= underground->attract_disembark_stop_index) {
                underground->button_pressed = true;
                underground->attract_route = UNDERGROUND_NO_STATION;
            }
        } else {
            underground->attract_route = UNDERGROUND_NO_STATION;
            underground->attract_disembark_stop_index = UNDERGROUND_NO_VIA_STOP_INDEX;
        }
    }
    FSM_Service(&game->fsm);
}

static Callbacks underground_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = underground_init_enter,
        .service = underground_init_service,
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = underground_startup_enter,
        .service = underground_startup_service,
    },
    [MODE_FSM_STATE_IDLE] = {0},
    [MODE_FSM_STATE_ATTRACT] = {
        .enter = underground_attract_enter,
        .service = underground_attract_service,
    },
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition underground_mode = {
    .state_callbacks = underground_state_callbacks,
    .always_service = underground_always_service,
};
