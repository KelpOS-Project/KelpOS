#include "app.h"

#include "../font.h"

void graphics_app_network_draw(const struct graphics_app_context *context)
{
    struct graphics_framebuffer *fb = context->framebuffer;
    graphics_fill_rect(fb, context->x, context->y,
        context->width, context->height, 0xC0C0C0);
    graphics_draw_text(fb, context->x + 18, context->y + 22,
        "NETWORK STATUS", 0x000080, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 52,
        "DEVICE: INTEL E1000 RAW ETHERNET", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 76,
        "IP: NOT CONFIGURED", 0x000000, 1);
    graphics_draw_text(fb, context->x + 18, context->y + 100,
        "TCP/IP AND WEB ACCESS NOT AVAILABLE", 0x404040, 1);
}