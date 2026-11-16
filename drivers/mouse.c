/* drivers/mouse.c — PS/2 mouse on the 8042 aux channel (IRQ12 -> 44).
 *
 * Enables the aux port, unmasked IRQ12, reads 3-byte stream packets
 * and exposes a polled API (position/buttons) for the desktop. */
#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "mouse.h"

#define CTRL   0x64
#define DATA   0x60

static int mx, my;         /* pixel position */
static int max_x, max_y;
static int btn;            /* latest buttons */
static volatile int moved; /* packet completed since last poll */
static volatile int click; /* left-button press edge */
static int prev_left;
static uint8_t pkt[3];
static int pktn;

static void wait_ib_empty(void)
{
    for (int i = 0; i < 100000 && (inb(CTRL) & 0x02); i++)
        ;
}

static void wait_ob_full(void)
{
    for (int i = 0; i < 100000 && !(inb(CTRL) & 0x01); i++)
        ;
}

/* send a command byte to the mouse (0xD4 prefix) */
static void mouse_send(uint8_t c)
{
    wait_ib_empty();
    outb(CTRL, 0xD4);
    wait_ib_empty();
    outb(DATA, c);
}

void mouse_init(int screen_w, int screen_h)
{
    mx = screen_w / 2;
    my = screen_h / 2;
    max_x = screen_w;
    max_y = screen_h;
    pktn = 0;
    moved = 0;
    click = 0;
    prev_left = 0;

    /* enable the aux (mouse) port */
    wait_ib_empty();
    outb(CTRL, 0xA8);

    /* read the controller config byte: enable IRQ12, enable mouse clock */
    wait_ib_empty();
    outb(CTRL, 0x20);
    wait_ob_full();
    uint8_t cfg = inb(DATA);
    cfg |= 0x02;   /* enable IRQ12 */
    cfg &= ~0x20;  /* enable mouse clock line */
    wait_ib_empty();
    outb(CTRL, 0x60);
    wait_ib_empty();
    outb(DATA, cfg);

    /* default settings, then enable data reporting (stream mode) */
    mouse_send(0xF6);
    mouse_send(0xF4);
}

void mouse_isr(void)
{
    uint8_t st = inb(CTRL);
    if (!(st & 0x01))
        return; /* spurious */
    uint8_t d = inb(DATA);
    if (!(st & 0x20))
        return; /* not from the aux channel */

    if (pktn == 0 && !(d & 0x08))
        return; /* resync: byte 0 always has bit 3 set */
    if (pktn == 0 && (d == 0xFA || d == 0xFE || d == 0xFF))
        return; /* mouse ACK / resend / self-test pass: not a packet */

    pkt[pktn++] = d;
    if (pktn < 3)
        return;
    pktn = 0;

    /* packet dy > 0 means "moved up". Deltas are 9-bit two's
     * complements: the low 8 bits are pkt[1]/pkt[2], the sign lives in
     * byte 0 (bit 4 = X sign, bit 5 = Y sign). Never cast to int8_t
     * first — that double-extends the sign and magnifies deltas. */
    int dx = pkt[1];
    int dy = pkt[2];
    if (pkt[0] & 0x10)
        dx -= 256;
    if (pkt[0] & 0x20)
        dy -= 256;

    int nx = mx + dx;
    int ny = my - dy; /* packet dy > 0 means "moved up" */
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx > max_x - 1) nx = max_x - 1;
    if (ny > max_y - 1) ny = max_y - 1;
    mx = nx;
    my = ny;

    btn = pkt[0] & 0x07;
    if (btn & 0x01 && !prev_left)
        click = 1;
    prev_left = btn & 0x01;
    moved = 1;
}

int mouse_x(void) { return mx; }
int mouse_y(void) { return my; }
int mouse_buttons(void) { return btn; }

int mouse_left_click(void)
{
    int r = click;
    click = 0;
    return r;
}

int mouse_moved(void)
{
    int r = moved;
    moved = 0;
    return r;
}
