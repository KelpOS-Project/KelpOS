/* kernel/desktop.c — basic graphical desktop on the framebuffer.
 *
 * Keyboard-driven (mouse support comes later): icon grid with Terminal,
 * Files, Browser, About, Reboot, Shutdown + a taskbar with clock.
 * Files browses the ramfs; Browser is an honest placeholder until the
 * network stack (roadmap M4/M5) exists. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "desktop.h"
#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "version.h"
#include "users.h"
#include "shell.h"
#include "ramfs.h"
#include "bin/commands.h"

/* palette */
#define COL_DESKTOP_BG 0x0e1c33u
#define COL_BAR_BG     0x16263fu
#define COL_ICON_BG    0x24406bu
#define COL_ICON_BG_SEL 0x3fa34du
#define COL_ICON_FG    0xd8e8ffu
#define COL_TITLE_FG   0x7fd4a8u
#define COL_TEXT_FG    0xd8d8d8u
#define COL_ACCENT     0x3fa34du

#define NAPPS 6
#define ICON_W 8

struct app {
    char glyph;
    const char *label;
};

static const struct app apps[NAPPS] = {
    { 'T', "Terminal" },
    { 'F', "Files" },
    { 'B', "Browser" },
    { 'A', "About" },
    { 'R', "Reboot" },
    { 'S', "Shutdown" },
};

static unsigned sel; /* selected icon */

/* ── drawing helpers ────────────────────────────────────────────────── */

static void put(int col, int row, char ch, uint32_t fg, uint32_t bg)
{
    console_draw_cell((unsigned)col, (unsigned)row, ch, fg, bg);
}

static void text_center(int row, const char *s, uint32_t fg, uint32_t bg)
{
    int cols = (int)console_cols();
    int len = (int)strlen(s);
    int start = (cols - len) / 2;
    for (int c = 0; c < cols; c++)
        put(c, row, ' ', fg, bg);
    for (int i = 0; i < len; i++)
        put(start + i, row, s[i], fg, bg);
}

static void draw_icon(int base_col, int base_row, const struct app *a, bool selected)
{
    uint32_t bg = selected ? COL_ICON_BG_SEL : COL_ICON_BG;
    uint32_t fg = selected ? 0x0e1c33u : COL_ICON_FG;
    /* 8x3 box with the glyph centered inside */
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < ICON_W; c++) {
            int edge = (r == 0 || r == 2 || c == 0 || c == ICON_W - 1);
            int center = (r == 1 && c == ICON_W / 2);
            put(base_col + c, base_row + r,
                edge || !center ? ' ' : a->glyph, fg, bg);
        }
    /* label centered under the box, clipped to the box width */
    int len = (int)strlen(a->label);
    if (len > ICON_W)
        len = ICON_W;
    int off = (ICON_W - len) / 2;
    for (int c = 0; c < ICON_W; c++) {
        int i = c - off;
        put(base_col + c, base_row + 4,
            i >= 0 && i < len ? a->label[i] : ' ',
            selected ? COL_TITLE_FG : COL_TEXT_FG, COL_DESKTOP_BG);
    }
}

static void draw_desktop(void)
{
    unsigned cols = console_cols(), rows = console_rows();
    console_fill(0, 0, cols - 1, rows - 1, COL_DESKTOP_BG);

    text_center(1, "KelpOS " KELPOS_VERSION " Desktop", COL_TITLE_FG, COL_DESKTOP_BG);

    /* 3x2 icon grid */
    int grid_w = 3 * 10;                 /* 6-cell icons + 4 gap */
    int start_col = ((int)cols - grid_w) / 2 + 2;
    int start_row = (int)rows / 2 - 8;
    for (int i = 0; i < NAPPS; i++) {
        int gx = i % 3, gy = i / 3;
        draw_icon(start_col + gx * 10, start_row + gy * 6, &apps[i], i == (int)sel);
    }

    /* taskbar */
    unsigned bar_row = rows - 1;
    console_fill(0, bar_row, cols - 1, bar_row, COL_BAR_BG);
    char left[64], right[64];
    snprintf(left, sizeof(left), " %s", user_current()->name);
    snprintf(right, sizeof(right), "%llu s  KelpOS ",
             (unsigned long long)(timer_ms() / 1000));
    for (unsigned c = 0; c < cols; c++)
        put((int)c, (int)bar_row, ' ', COL_TEXT_FG, COL_BAR_BG);
    for (size_t i = 0; i < strlen(left) && i < cols; i++)
        put((int)i, (int)bar_row, left[i], COL_TEXT_FG, COL_BAR_BG);
    size_t rl = strlen(right);
    for (size_t i = 0; i < rl && i < cols; i++)
        put((int)(cols - rl + i), (int)bar_row, right[i], COL_TEXT_FG, COL_BAR_BG);
}

static void redraw(void)
{
    draw_desktop();
}

/* ── modal window (Files viewer, Browser, About) ────────────────────── */

static void window_box(int c0, int r0, int c1, int r1, const char *title)
{
    console_fill((unsigned)c0, (unsigned)r0, (unsigned)c1, (unsigned)r1, 0x0b1220u);
    for (int c = c0; c <= c1; c++) {
        put(c, r0, '-', COL_TITLE_FG, 0x0b1220u);
        put(c, r1, '-', COL_TITLE_FG, 0x0b1220u);
    }
    for (int r = r0 + 1; r < r1; r++) {
        put(c0, r, '|', COL_TITLE_FG, 0x0b1220u);
        put(c1, r, '|', COL_TITLE_FG, 0x0b1220u);
    }
    put(c0, r0, '+', COL_TITLE_FG, 0x0b1220u);
    put(c1, r0, '+', COL_TITLE_FG, 0x0b1220u);
    put(c0, r1, '+', COL_TITLE_FG, 0x0b1220u);
    put(c1, r1, '+', COL_TITLE_FG, 0x0b1220u);
    for (size_t i = 0; i < strlen(title) && (int)i < c1 - c0 - 3; i++)
        put(c0 + 2 + (int)i, r0, title[i], COL_TITLE_FG, 0x0b1220u);
}

static void show_text_window(const char *title, const char *const *lines, int n)
{
    unsigned cols = console_cols(), rows = console_rows();
    int w = (int)cols * 3 / 5, h = (int)rows / 2;
    int c0 = ((int)cols - w) / 2, r0 = ((int)rows - h) / 2;
    window_box(c0, r0, c0 + w - 1, r0 + h - 1, title);
    for (int i = 0; i < n && r0 + 2 + i < r0 + h - 1; i++) {
        const char *s = lines[i];
        for (int j = 0; s[j] && c0 + 2 + j < c0 + w - 2; j++)
            put(c0 + 2 + j, r0 + 2 + i, s[j], COL_TEXT_FG, 0x0b1220u);
    }
    /* wait for any key */
    kbd_getchar();
}

/* ── Files app ──────────────────────────────────────────────────────── */

static void files_view_file(struct ramfs_node *f)
{
    char linebuf[16][72];
    const char *lines[16];
    const char *data = ramfs_data(f);
    int n = 0, pos = 0;

    while (n < 15 && data[pos]) {
        size_t len = 0;
        while (data[pos + len] && data[pos + len] != '\n' && len < 70)
            len++;
        memcpy(linebuf[n], data + pos, len);
        linebuf[n][len] = 0;
        lines[n] = linebuf[n];
        n++;
        pos += (int)len;
        if (data[pos] == '\n')
            pos++;
    }
    char tmp[72];
    snprintf(tmp, sizeof(tmp), "%s (%llu B)",
             ramfs_node_name(f), (unsigned long long)ramfs_size(f));
    show_text_window(tmp, (const char *const *)lines, n);
}

static void files_app(void)
{
    static char path[96] = "/";
    int cur = 0;

    for (;;) {
        unsigned cols = console_cols(), rows = console_rows();
        console_fill(0, 0, cols - 1, rows - 1, 0x0b1220u);
        text_center(1, "Files", COL_TITLE_FG, 0x0b1220u);
        text_center(2, path, COL_TEXT_FG, 0x0b1220u);

        struct ramfs_node *d = ramfs_resolve_at(ramfs_root(), path);
        struct ramfs_node *kids[24];
        size_t n = d ? ramfs_children(d, kids, 24) : 0;
        if (n == 0)
            text_center(4, "(empty)", 0x667788u, 0x0b1220u);
        for (size_t i = 0; i < n && 4 + (int)i < (int)rows - 2; i++) {
            uint32_t fg = ramfs_is_dir(kids[i]) ? 0x7fb3d5u : COL_TEXT_FG;
            char line[80];
            snprintf(line, sizeof(line), " %s%s",
                     ramfs_node_name(kids[i]), ramfs_is_dir(kids[i]) ? "/" : "");
            int row = 4 + (int)i;
            for (unsigned c = 0; c < cols; c++)
                put((int)c, row, ' ', fg, 0x0b1220u);
            for (size_t j = 0; line[j] && j < cols; j++)
                put((int)j, row, line[j], fg, (int)i == cur ? 0x24406bu : 0x0b1220u);
        }
        text_center((int)rows - 2, "arrows: move  enter: open  esc: up  q: quit",
                    0x667788u, 0x0b1220u);

        int c = kbd_getchar();
        if (c == 'q' || c == 'Q')
            return;
        if (c == KBD_UP && cur > 0)
            cur--;
        if (c == KBD_DOWN && cur < (int)n - 1)
            cur++;
        if (c == KBD_ESC) {
            if (strcmp(path, "/") != 0) {
                char *slash = strrchr(path, '/');
                if (slash && slash != path)
                    *slash = 0;
                else
                    strcpy(path, "/");
                cur = 0;
            }
            continue;
        }
        if (c == '\n' && n > 0 && cur < (int)n) {
            if (ramfs_is_dir(kids[cur])) {
                if (strcmp(path, "/") != 0)
                    strncat(path, "/", sizeof(path) - strlen(path) - 1);
                strncat(path, ramfs_node_name(kids[cur]),
                        sizeof(path) - strlen(path) - 1);
                cur = 0;
            } else {
                files_view_file(kids[cur]);
            }
        }
    }
}

/* ── main desktop loop ──────────────────────────────────────────────── */

void desktop_run(void)
{
    if (!console_fb_active()) {
        /* no framebuffer: fall back to the plain shell */
        kprintf("Desktop needs a framebuffer — starting shell instead.\n");
        shell_run();
    }

    sel = 0;
    redraw();

    for (;;) {
        int c = kbd_getchar();
        switch (c) {
        case KBD_LEFT:
            if (sel % 3 > 0)
                sel--;
            break;
        case KBD_RIGHT:
            if (sel % 3 < 2 && sel + 1 < NAPPS)
                sel++;
            break;
        case KBD_UP:
            if (sel >= 3)
                sel -= 3;
            break;
        case KBD_DOWN:
            if (sel + 3 < NAPPS)
                sel += 3;
            break;
        case '\n':
            switch (sel) {
            case 0: /* Terminal */
                console_set_colors(KELP_FG, KELP_BG);
                console_clear();
                shell_run(); /* noreturn */
            case 1: /* Files */
                files_app();
                break;
            case 2: /* Browser */
                show_text_window("Browser", (const char *const[]){
                    "KelpOS Web Browser", "",
                    "No network stack yet — the browser ships once",
                    "TCP/IP + sockets land (roadmap M5).", "",
                    "Until then: the sea is calm.", "",
                    "Press any key to return."}, 9);
                break;
            case 3: /* About */
                show_text_window("About", (const char *const[]){
                    "KelpOS " KELPOS_VERSION " (x86_64)", "",
                    "A free (GPL-3.0) Unix-like hobby kernel.",
                    "Boot: Limine Boot Protocol",
                    "Language: C + optional Rust",
                    "Developed with AI assistance.",
                    "",
                    "Press any key to return."}, 8);
                break;
            case 4: /* Reboot */
                cmd_reboot(0, 0);
                __attribute__((fallthrough)); /* cmd_reboot never returns */
            case 5: /* Shutdown */
                cmd_shutdown(0, 0);
                break;
            }
            break;
        default:
            continue; /* ignore stray keys */
        }
        redraw();
    }
}

/* the `desktop` shell command re-enters the graphical desktop */
int cmd_desktop(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_set_colors(KELP_FG, KELP_BG);
    console_clear();
    desktop_run();
}
