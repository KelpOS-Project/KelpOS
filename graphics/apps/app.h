#ifndef KELPOS_GRAPHICS_APPS_APP_H
#define KELPOS_GRAPHICS_APPS_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../framebuffer.h"
#include "../game_engine.h"

enum desktop_app {
    DESKTOP_APP_NONE,
    DESKTOP_APP_TERMINAL,
    DESKTOP_APP_EXPLORER,
    DESKTOP_APP_BROWSER,
    DESKTOP_APP_GAME,
    DESKTOP_APP_SYSTEM,
    DESKTOP_APP_NETWORK,
    DESKTOP_APP_STORAGE,
    DESKTOP_APP_COUNT,
};

struct graphics_app_info {
    enum desktop_app id;
    const char *title;
    const char *icon;
    uint32_t color;
};

struct graphics_app_context {
    struct graphics_framebuffer *framebuffer;
    size_t x;
    size_t y;
    size_t width;
    size_t height;
    const char *const *lines;
    size_t line_count;
    const char *input;
    struct game_engine *game;
};

const struct graphics_app_info *graphics_apps(size_t *count);
const struct graphics_app_info *graphics_app_info(enum desktop_app app);
void graphics_app_draw(enum desktop_app app, const struct graphics_app_context *context);
bool graphics_app_mouse_click(enum desktop_app app,
    const struct graphics_app_context *context, size_t x, size_t y);

#endif