#include "app.h"

#include "../font.h"

void graphics_app_explorer_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xFFFFFF);
    graphics_fill_rect(fb, context->x, context->y, context->width, 20, 0xC0C0C0);
    graphics_draw_text(fb, context->x + 7, context->y + 7,
        "FILE EDIT VIEW HELP", 0x000000, 1);
    graphics_fill_rect(fb, context->x + 8, context->y + 28,
        context->width - 16, 20, 0xFFFFFF);
    graphics_fill_rect(fb, context->x + 8, context->y + 28,
        context->width - 16, 1, 0x808080);
    graphics_draw_text(fb, context->x + 14, context->y + 35,
        "ADDRESS: MY COMPUTER", 0x000000, 1);
    graphics_fill_rect(fb, context->x + 8, context->y + 56,
        context->width - 16, context->height - 64, 0xFFFFFF);
    graphics_draw_text(fb, context->x + 22, context->y + 72,
        "MY COMPUTER", 0x000080, 1);
    graphics_draw_text(fb, context->x + 22, context->y + 102,
        "A: FLOPPY DRIVE - NOT PRESENT", 0x000000, 1);
    graphics_draw_text(fb, context->x + 22, context->y + 126,
        "C: LOCAL DISK - NOT AVAILABLE", 0x000000, 1);
    graphics_draw_text(fb, context->x + 22, context->y + 160,
        "NO BLOCK DEVICE OR FILESYSTEM IS MOUNTED", 0x404040, 1);
}