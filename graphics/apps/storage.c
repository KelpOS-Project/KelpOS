#include "app.h"

#include "../font.h"

void graphics_app_storage_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xC0C0C0);
    graphics_draw_text(fb, context->x + 18, context->y + 22,
        "DISK STATUS", 0x000080, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 52,
        "BLOCK DEVICES: NONE", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 76,
        "FILESYSTEM: NOT MOUNTED", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 100,
        "EXT4: NOT AVAILABLE", 0x404040, 1);
}