/* kernel/tui.c — ncurses-style widget layer on the text console.
 *
 * Everything is drawn as color cells through console_draw_cell(), so
 * the layer works with the same 8x16 font grid as the shell and needs
 * no terminfo, no libc and no floating point. The visual language
 * (title bar, arrow-key menus, highlighted rows, help line, buttons)
 * follows Linux `make menuconfig`.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "console.h"
#include "keyboard.h"
#include "tui.h"

#define TUI_BG       0x0d1626u
#define TUI_FG       0xc9d6e8u
#define TUI_DIM      0x6f8098u
#define TUI_BORDER   0x2b4668u
#define TUI_TITLE_BG 0x24507du
#define TUI_TITLE_FG 0xf2f7ffu
#define TUI_SEL_BG   0x2f6db2u
#define TUI_SEL_FG   0xffffffffu
#define TUI_VALUE    0xf0d18cu
#define TUI_ACCENT   0x7fd4ffu
#define TUI_HINT     0x9fb4ccu
#define TUI_FOOT_BG  0x16233au
#define TUI_FOOT_FG  0xa9bdd6u
#define TUI_BAR_BG   0x1a2740u
#define TUI_BAR_FG   0x59b6efu
#define TUI_MARK     0xffd75eu
#define TUI_BTN_BG   0x1d3a5cu

int tui_cols(void) { return (int)console_cols(); }
int tui_rows(void) { return (int)console_rows(); }

bool tui_available(void)
{
    return console_fb_active() && tui_cols() >= 64 && tui_rows() >= 20;
}

void tui_begin(void)
{
    console_fb_suspend(true);
    console_clear();
}

void tui_end(void)
{
    console_clear();
    console_fb_suspend(false);
}

void tui_put(int col, int row, const char *s, uint32_t fg, uint32_t bg)
{
    if (!s || row < 0 || row >= tui_rows())
        return;
    for (; *s; s++, col++) {
        if (col < 0)
            continue;
        if (col >= tui_cols())
            break;
        console_draw_cell((unsigned)col, (unsigned)row, *s, fg, bg);
    }
}

void tui_fill(int col0, int row0, int col1, int row1, uint32_t bg)
{
    if (col0 < 0)
        col0 = 0;
    if (row0 < 0)
        row0 = 0;
    if (col1 >= tui_cols())
        col1 = tui_cols() - 1;
    if (row1 >= tui_rows())
        row1 = tui_rows() - 1;
    if (col0 > col1 || row0 > row1)
        return;
    console_fill((unsigned)col0, (unsigned)row0, (unsigned)col1,
                 (unsigned)row1, bg);
}

static void hline(int row, uint32_t fg)
{
    for (int c = 0; c < tui_cols(); c++)
        console_draw_cell((unsigned)c, (unsigned)row, '-', fg, TUI_BG);
}

static int text_col(int width)
{
    int col = (tui_cols() - width) / 2;
    return col < 0 ? 0 : col;
}

/* Full-screen chrome: title bar, separators, footer hint bar. */
static void frame(const char *title, const char *footer)
{
    int c = tui_cols(), r = tui_rows();
    tui_fill(0, 0, c - 1, r - 1, TUI_BG);

    tui_fill(0, 0, c - 1, 0, TUI_TITLE_BG);
    tui_put(2, 0, title, TUI_TITLE_FG, TUI_TITLE_BG);

    hline(1, TUI_BORDER);
    hline(r - 2, TUI_BORDER);

    tui_fill(0, r - 1, c - 1, r - 1, TUI_FOOT_BG);
    tui_put(2, r - 1, footer ? footer : "Up/Down: select   Enter: choose   "
            "Esc: back", TUI_FOOT_FG, TUI_FOOT_BG);
}

static int first_enabled(const struct tui_item *items, int count)
{
    for (int i = 0; i < count; i++)
        if (items[i].label && !items[i].disabled)
            return i;
    return -1;
}

static int next_enabled(const struct tui_item *items, int count, int from,
                        int dir)
{
    for (int i = from + dir; i >= 0 && i < count; i += dir)
        if (items[i].label && !items[i].disabled)
            return i;
    return from;
}

static void draw_menu(const char *title, const char *subtitle,
                      const struct tui_item *items, int count, int sel,
                      const char *footer)
{
    frame(title, footer);
    int c = tui_cols(), r = tui_rows();

    if (subtitle)
        tui_put(3, 2, subtitle, TUI_DIM, TUI_BG);

    /* value column: a fixed second column like menuconfig, pushed
     * right only when the longest label would collide with it */
    int label_w = 0;
    for (int i = 0; i < count; i++)
        if (items[i].label) {
            int w = (int)strlen(items[i].label);
            if (w > label_w)
                label_w = w;
        }
    int value_col = 5 + label_w + 6;
    if (value_col > c / 2)
        value_col = c / 2;

    int top = 4;
    for (int i = 0; i < count; i++) {
        int row = top + i;
        if (row > r - 4)
            break;
        const struct tui_item *it = &items[i];
        if (!it->label) {
            hline(row, TUI_BORDER);
            continue;
        }

        bool on = (i == sel);
        uint32_t bg = on ? TUI_SEL_BG : TUI_BG;
        uint32_t fg = it->disabled ? TUI_DIM : TUI_FG;

        if (on)
            tui_fill(2, row, c - 3, row, TUI_SEL_BG);
        tui_put(3, row, on ? ">" : " ", on ? TUI_MARK : TUI_BG, bg);
        tui_put(5, row, it->label, fg, bg);

        if (it->value) {
            int vcol = value_col;
            if (vcol < 5 + (int)strlen(it->label) + 2)
                vcol = 5 + (int)strlen(it->label) + 2;
            if (vcol + (int)strlen(it->value) > c - 4)
                vcol = c - 4 - (int)strlen(it->value);
            tui_put(vcol, row, it->value,
                    on ? TUI_SEL_FG
                       : (it->disabled ? TUI_DIM : TUI_VALUE), bg);
        }
    }

    const char *hint = sel >= 0 && sel < count ? items[sel].hint : 0;
    if (hint)
        tui_put(3, r - 3, hint, TUI_HINT, TUI_BG);
}

int tui_menu(const char *title, const char *subtitle,
             const struct tui_item *items, int count, int sel,
             const char *footer)
{
    if (!items || count <= 0)
        return -1;

    if (first_enabled(items, count) < 0)
        return -1;
    if (sel < 0 || sel >= count || !items[sel].label || items[sel].disabled) {
        sel = first_enabled(items, count);
    }

    for (;;) {
        draw_menu(title, subtitle, items, count, sel, footer);
        int k = kbd_getchar();

        switch (k) {
        case KBD_UP:
        case 'k':
            sel = next_enabled(items, count, sel, -1);
            break;
        case KBD_DOWN:
        case 'j':
        case '\t':
            sel = next_enabled(items, count, sel, +1);
            break;
        case KBD_HOME:
            sel = first_enabled(items, count);
            break;
        case KBD_END: {
            int i = first_enabled(items, count);
            int last = i;
            while (i >= 0) {
                last = i;
                i = next_enabled(items, count, i, +1);
                if (i == last)
                    break;
            }
            if (last >= 0)
                sel = last;
            break;
        }
        case '\n':
        case '\r':
            if (!items[sel].disabled)
                return sel;
            break;
        case KBD_ESC:
            return -1;
        default:
            break;
        }
    }
}

int tui_page(const char *title, const char *heading,
             const char *const *lines, int count, const char *footer)
{
    frame(title, footer ? footer : "Enter: back");
    int r = tui_rows();
    int row = 3;

    if (heading) {
        tui_put(4, row, heading, TUI_ACCENT, TUI_BG);
        row += 2;
    }
    for (int i = 0; i < count && row <= r - 3; i++, row++)
        tui_put(4, row, lines[i], TUI_FG, TUI_BG);

    for (;;) {
        int k = kbd_getchar();
        if (k == '\n' || k == '\r' || k == ' ' || k == KBD_ESC)
            return k;
    }
}

static void button(int col, int row, const char *label, bool on)
{
    int w = (int)strlen(label) + 4;
    uint32_t bg = on ? TUI_SEL_BG : TUI_BTN_BG;
    uint32_t fg = on ? TUI_SEL_FG : TUI_FG;
    tui_fill(col, row, col + w - 1, row, bg);
    tui_put(col + 2, row, label, fg, bg);
}

bool tui_confirm(const char *title, const char *heading,
                 const char *const *lines, int count,
                 const char *ok, const char *cancel)
{
    frame(title, "Left/Right: choose   Enter: confirm   Esc: cancel");
    int r = tui_rows();
    int row = 3;

    if (heading) {
        tui_put(4, row, heading, TUI_ACCENT, TUI_BG);
        row += 2;
    }
    for (int i = 0; i < count && row <= r - 6; i++, row++)
        tui_put(4, row, lines[i], TUI_FG, TUI_BG);

    int wok = (int)strlen(ok) + 4;
    int wcancel = (int)strlen(cancel) + 4;
    int total = wok + 6 + wcancel;
    int col = text_col(total);
    int ok_col = col;
    int cancel_col = col + wok + 6;
    int btn_row = r - 4;

    int choice = 1; /* default: the safe button */
    for (;;) {
        button(ok_col, btn_row, ok, choice == 0);
        button(cancel_col, btn_row, cancel, choice == 1);

        int k = kbd_getchar();
        switch (k) {
        case KBD_LEFT:
        case KBD_RIGHT:
        case '\t':
            choice ^= 1;
            break;
        case '\n':
        case '\r':
            return choice == 0;
        case KBD_ESC:
            return false;
        default:
            break;
        }
    }
}

void tui_progress(const char *title, const char *stage, unsigned pct,
                  const char *detail)
{
    if (pct > 100)
        pct = 100;
    frame(title, "Installing - do not power off");

    int c = tui_cols(), r = tui_rows();
    int row = r / 2;

    tui_put(3, 3, stage, TUI_ACCENT, TUI_BG);

    int barw = c - 16;
    int x = 8;
    int fill = (int)(((long)barw * (long)pct) / 100);

    console_draw_cell((unsigned)x, (unsigned)row, '[', TUI_BAR_FG, TUI_BG);
    for (int i = 0; i < barw; i++) {
        bool on = i < fill;
        console_draw_cell((unsigned)(x + 1 + i), (unsigned)row,
                          on ? '=' : ' ',
                          on ? TUI_BAR_FG : TUI_BAR_BG,
                          on ? TUI_BAR_BG : TUI_BAR_BG);
    }
    console_draw_cell((unsigned)(x + 1 + barw), (unsigned)row, ']',
                      TUI_BAR_FG, TUI_BG);

    char pctstr[16];
    snprintf(pctstr, sizeof(pctstr), "%u%%", pct);
    tui_put(text_col((int)strlen(pctstr)), row + 2, pctstr, TUI_FG, TUI_BG);

    if (detail)
        tui_put(text_col((int)strlen(detail)), row + 3, detail, TUI_DIM, TUI_BG);
}
