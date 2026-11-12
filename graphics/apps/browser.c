#include "app.h"

#include "../font.h"

void graphics_app_browser_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xFFFFFF);
    graphics_fill_rect(fb, context->x, context->y, context->width, 20, 0xC0C0C0);
    graphics_draw_text(fb, context->x + 7, context->y + 7,
        "FILE EDIT VIEW FAVORITES HELP", 0x000000, 1);
    graphics_fill_rect(fb, context->x + 8, context->y + 28,
        context->width - 16, 20, 0xFFFFFF);
    graphics_draw_text(fb, context->x + 14, context->y + 35,
        "ADDRESS: ABOUT:KELPOS", 0x000000, 1);
    graphics_draw_text(fb, context->x + 24, context->y + 84,
        "INTERNET EXPLORER", 0x000080, 2);
    graphics_draw_text(fb, context->x + 24, context->y + 126,
        "NO TCP/IP STACK IS AVAILABLE", 0x000000, 1);
    graphics_draw_text(fb, context->x + 24, context->y + 150,
        "NETWORKING CURRENTLY SUPPORTS RAW ETHERNET ONLY", 0x404040, 1);
}