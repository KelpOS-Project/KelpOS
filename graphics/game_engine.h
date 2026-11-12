#ifndef KELPOS_GRAPHICS_GAME_ENGINE_H
#define KELPOS_GRAPHICS_GAME_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

enum game_engine_result {
    GAME_ENGINE_PLAYING,
    GAME_ENGINE_X_WON,
    GAME_ENGINE_O_WON,
    GAME_ENGINE_DRAW,
};

struct game_engine {
    char cells[9];
    char current_player;
    enum game_engine_result result;
    uint8_t move_count;
};

void game_engine_reset(struct game_engine *game);
bool game_engine_play(struct game_engine *game, unsigned int row,
    unsigned int column);

#endif