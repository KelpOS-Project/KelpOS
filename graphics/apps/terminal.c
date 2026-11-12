#include "app.h"

#include "../font.h"

void graphics_app_terminal_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0x000000);
    const size_t row_height = 11;
    size_t visible = context->height > 28
        ? (context->height - 28) / row_height : 0;
    if (visible > context->line_count) visible = context->line_count;
    for (size_t row = 0; row < visible; row++) {
        graphics_draw_text(fb, context->x + 8,
            context->y + 6 + row * row_height,
            context->lines[context->line_count - visible + row], 0xC0C0C0, 1);
    }
    const size_t prompt_y = context->y + context->height - 17;
    graphics_draw_text(fb, context->x + 8, prompt_y, "C:\\KELP>", 0xFFFFFF, 1);
    graphics_draw_text(fb, context->x + 56, prompt_y, context->input, 0xFFFFFF, 1);
}