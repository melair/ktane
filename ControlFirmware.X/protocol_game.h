#ifndef PROTOCOL_GAME_H
#define	PROTOCOL_GAME_H

void protocol_game_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_game_state_send(uint8_t state, uint32_t seed, uint8_t strikes_current, uint8_t strikes, uint8_t minutes, uint8_t seconds, uint8_t centiseconds, uint8_t time_ratio);
void protocol_game_module_config_send(uint8_t target_id, bool enabled, uint8_t difficulty);
void protocol_game_module_state_send(bool ready, bool solved);
void protocol_game_module_strike_send(uint8_t strikes);

#endif	/* PROTOCOL_GAME_H */

