#ifndef UNDERGROUND_H
#define UNDERGROUND_H

#include "mode_fsm.h"
#include "input_manager/input_manager.h"
#include "display/ssd1322/ssd1322.h"
#include "mode/puzzle/underground/underground_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNDERGROUND_GAME_FSM_STATE_AT_STATION = 0,
    UNDERGROUND_GAME_FSM_STATE_ON_TRAIN,
    UNDERGROUND_GAME_FSM_STATE_FORCIBLY_DISEMBARKED,
    UNDERGROUND_GAME_FSM_STATE_COUNT,
} Underground_Game_FSM_State;

typedef struct {
    FSM fsm;
    UndergroundStationID station;
    uint32_t seconds;
    UndergroundRouteID arriving_route;
    uint32_t last_arrival_seconds;
    uint32_t route_next_arrival_seconds[UNDERGROUND_ROUTE_COUNT];
    UndergroundRouteID route;
    uint8_t route_stop_index;
    bool train_at_station;
    uint32_t train_next_move_seconds;
    bool ejected_out_of_zone;
    uint32_t ejected_at_seconds;
    bool ejection_started;
} Underground_Game;

typedef struct {
    Underground_Game game;
    SSD1322 display;
    uint8_t framebuffer[SSD1322_FRAMEBUFFER_SIZE];
    bool display_init_failed;
    uint32_t startup_started_ms;
    bool startup_clear_started;
    uint32_t attract_last_input_ms;
    UndergroundRouteID attract_route;
    uint8_t attract_disembark_stop_index;
    IM_EventQueue button_queue;
    IM_DigitalChannelState button_channel_state[1];
    IM_DigitalInputState button_input_state;
    IM_DigitalInputConfig button_input_config;
    IM_Handle button_handle;
    bool button_pressed;
} Underground_Data;

extern Mode_Definition underground_mode;

#ifdef __cplusplus
}
#endif

#endif //UNDERGROUND_H
