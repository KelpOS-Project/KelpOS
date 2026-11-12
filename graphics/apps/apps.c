#include "app.h"

#include "../font.h"

void graphics_app_terminal_draw(const struct graphics_app_context *context);
void graphics_app_explorer_draw(const struct graphics_app_context *context);
void graphics_app_browser_draw(const struct graphics_app_context *context);
void graphics_app_game_draw(const struct graphics_app_context *context);
void graphics_app_system_draw(const struct graphics_app_context *context);
void graphics_app_network_draw(const struct graphics_app_context *context);
void graphics_app_storage_draw(const struct graphics_app_context *context);
bool graphics_app_game_click(const struct graphics_app_context *context,
    size_t x, size_t y);

static const struct graphics_app_info app_list[] = {
    { DESKTOP_APP_TERMINAL, "MS-DOS PROMPT", ">_", 0x008080 },
    { DESKTOP_APP_EXPLORER, "WINDOWS EXPLORER", "PC", 0x0000AA },
    { DESKTOP_APP_BROWSER, "INTERNET EXPLORER", "IE", 0x0000AA },
    { DESKTOP_APP_GAME, "KELP GAMES", "G", 0x008000 },
    { DESKTOP_APP_SYSTEM, "SYSTEM PROPERTIES", "PC", 0x808080 },
    { DESKTOP_APP_NETWORK, "NETWORK STATUS", "N", 0x008080 },
    { DESKTOP_APP_STORAGE, "DISK STATUS", "A:", 0xC08000 },
};

const struct graphics_app_info *graphics_apps(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(app_list) / sizeof(app_list[0]);
    }
    return app_list;
}

const struct graphics_app_info *graphics_app_info(enum desktop_app app)
{
    for (size_t index = 0; index < sizeof(app_list) / sizeof(app_list[0]); index++) {
        if (app_list[index].id == app) {
            return &app_list[index];
        }
    }
    return NULL;
}

void graphics_app_draw(enum desktop_app app, const struct graphics_app_context *context)
{
    if (context == NULL || context->framebuffer == NULL) {
        return;
    }
    switch (app) {
    case DESKTOP_APP_TERMINAL: graphics_app_terminal_draw(context); break;
    case DESKTOP_APP_EXPLORER: graphics_app_explorer_draw(context); break;
    case DESKTOP_APP_BROWSER: graphics_app_browser_draw(context); break;
    case DESKTOP_APP_GAME: graphics_app_game_draw(context); break;
    case DESKTOP_APP_SYSTEM: graphics_app_system_draw(context); break;
    case DESKTOP_APP_NETWORK: graphics_app_network_draw(context); break;
    case DESKTOP_APP_STORAGE: graphics_app_storage_draw(context); break;
    default: break;
    }
}

bool graphics_app_mouse_click(enum desktop_app app,
    const struct graphics_app_context *context, size_t x, size_t y)
{
    if (app == DESKTOP_APP_GAME) {
        return graphics_app_game_click(context, x, y);
    }
    return false;
}