#include "game_engine.h"

static enum game_engine_result find_result(const struct game_engine *game)
{
    static const uint8_t triples[8][3] = {
        { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 },
        { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 },
        { 0, 4, 8 }, { 2, 4, 6 },
    };
    for (unsigned int index = 0; index < 8; index++) {
        const char mark = game->cells[triples[index][0]];
        if (mark != 0 && mark == game->cells[triples[index][1]]
                && mark == game->cells[triples[index][2]]) {
            return mark == 'X' ? GAME_ENGINE_X_WON : GAME_ENGINE_O_WON;
        }
    }
    return game->move_count == 9 ? GAME_ENGINE_DRAW : GAME_ENGINE_PLAYING;
}

void game_engine_reset(struct game_engine *game)
{
    for (unsigned int index = 0; index < 9; index++) {
        game->cells[index] = 0;
    }
    game->current_player = 'X';
    game->result = GAME_ENGINE_PLAYING;
    game->move_count = 0;
}

bool game_engine_play(struct game_engine *game, unsigned int row,
    unsigned int column)
{
    if (game == NULL || row >= 3 || column >= 3
            || game->result != GAME_ENGINE_PLAYING) {
        return false;
    }
    const unsigned int cell = row * 3 + column;
    if (game->cells[cell] != 0) {
        return false;
    }
    game->cells[cell] = game->current_player;
    game->move_count++;
    game->result = find_result(game);
    if (game->result == GAME_ENGINE_PLAYING) {
        game->current_player = game->current_player == 'X' ? 'O' : 'X';
    }
    return true;
}