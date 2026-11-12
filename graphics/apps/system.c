#include "app.h"

#include "../font.h"

void graphics_app_system_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xC0C0C0);
    graphics_draw_text(fb, context->x + 18, context->y + 22,
        "KELPOS SYSTEM PROPERTIES", 0x000080, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 52,
        "PROCESSOR: X86_64", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 76,
        "MEMORY: 64 KB BOOTSTRAP HEAP", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 100,
        "BOOT: GRUB MULTIBOOT V1", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 124,
        "MODE: SINGLE-CORE PROTOTYPE", 0x000000, 1);
}