#include "app.h"

#include "../font.h"

static size_t board_size(const struct graphics_app_context *context)
{
    size_t size = context->width < context->height - 44
        ? context->width - 32 : context->height - 44;
    return size > 210 ? 210 : size;
}

static const char *result_text(const struct game_engine *game)
{
    switch (game->result) {
    case GAME_ENGINE_X_WON: return "X WINS!";
    case GAME_ENGINE_O_WON: return "O WINS!";
    case GAME_ENGINE_DRAW: return "DRAW GAME";
    default: return "CLICK A SQUARE TO PLAY";
    }
}

void graphics_app_game_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xC0C0C0);
    if (context->game == NULL || context->width < 90 || context->height < 110) {
        graphics_draw_text(fb, context->x + 8, context->y + 8,
            "WINDOW TOO SMALL", 0x000000, 1);
        return;
    }

    const size_t size = board_size(context);
    const size_t cell = size / 3;
    const size_t board_x = context->x + (context->width - size) / 2;
    const size_t board_y = context->y + 24;
    graphics_draw_text(fb, context->x + 12, context->y + 8,
        result_text(context->game), 0x000000, 1);
    graphics_fill_rect(fb, board_x, board_y, size, size, 0xFFFFFF);
    for (size_t line = 1; line < 3; line++) {
        graphics_fill_rect(fb, board_x + line * cell - 1,
            board_y, 2, size, 0x000000);
        graphics_fill_rect(fb, board_x,
            board_y + line * cell - 1, size, 2, 0x000000);
    }
    for (unsigned int row = 0; row < 3; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            const char mark = context->game->cells[row * 3 + column];
            if (mark != 0) {
                char text[] = { mark, '\0' };
                graphics_draw_text(fb,
                    board_x + column * cell + cell / 2 - 6,
                    board_y + row * cell + cell / 2 - 4,
                    text, mark == 'X' ? 0x0000AA : 0xAA0000, 2);
            }
        }
    }
    const size_t button_y = context->y + context->height - 28;
    graphics_fill_rect(fb, context->x + context->width - 82,
        button_y, 72, 22, 0xC0C0C0);
    graphics_fill_rect(fb, context->x + context->width - 82,
        button_y, 72, 1, 0xFFFFFF);
    graphics_fill_rect(fb, context->x + context->width - 82,
        button_y, 1, 22, 0xFFFFFF);
    graphics_fill_rect(fb, context->x + context->width - 82,
        button_y + 21, 72, 1, 0x404040);
    graphics_fill_rect(fb, context->x + context->width - 11,
        button_y, 1, 22, 0x404040);
    graphics_draw_text(fb, context->x + context->width - 72,
        button_y + 7, "NEW GAME", 0x000000, 1);
}

bool graphics_app_game_click(const struct graphics_app_context *context,
    size_t x, size_t y)
{
    if (context == NULL || context->game == NULL) {
        return false;
    }
    if (x >= context->width - 82 && x < context->width - 10
            && y >= context->height - 28 && y < context->height - 6) {
        game_engine_reset(context->game);
        return true;
    }
    const size_t size = board_size(context);
    const size_t board_x = (context->width - size) / 2;
    const size_t board_y = 24;
    if (x < board_x || x >= board_x + size || y < board_y || y >= board_y + size) {
        return false;
    }
    const unsigned int column = (unsigned int)((x - board_x) / (size / 3));
    const unsigned int row = (unsigned int)((y - board_y) / (size / 3));
    return game_engine_play(context->game, row, column);
}