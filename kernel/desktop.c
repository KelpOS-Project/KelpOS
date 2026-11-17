/* kernel/desktop.c — KelpOS desktop: a small window manager.
 *
 * Mouse-driven (drivers/mouse.c), in the spirit of classic desktops:
 *   - wallpaper with a diagonal two-tone pattern,
 *   - overlapping windows with title bars, a close box and dragging,
 *   - a taskbar with a start button, one button per open window and
 *     a real-time clock (CMOS RTC),
 *   - a start menu with all apps.
 *
 * Windows: Files (ramfs browser), Browser (honest placeholder until
 * the network stack lands) and About. Terminal is a fullscreen app:
 * the shell owns the whole cell grid, so it launches out of the WM.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "desktop.h"
#include "console.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "rtc.h"
#include "version.h"
#include "users.h"
#include "shell.h"
#include "ramfs.h"
#include "bin/commands.h"

/* ── palette ────────────────────────────────────────────────────────── */
#define COL_WALL0   0x14264bu  /* wallpaper diagonal stripes */
#define COL_WALL1   0x18305cu
#define COL_WALL2   0x1c3a6eu
#define COL_WALL3   0x102344u
#define COL_TITLE   0x2f7d43u  /* focused title bar (kelp green) */
#define COL_TITLE_D 0x24303eu  /* unfocused title bar */
#define COL_FRAME   0x2a3542u  /* window border */
#define COL_PANEL   0x161d29u  /* window body */
#define COL_TASKBAR 0x10161fu
#define COL_BTN     0x222c3au  /* taskbar buttons */
#define COL_BTN_HOV 0x2c3a4eu
#define COL_START   0x3fa34du  /* start button */
#define COL_START_H 0x4cb85cu
#define COL_TEXT    0xd8dee9u
#define COL_DIM     0x77869eu
#define COL_DIR     0x6fb3e0u
#define COL_SEL     0x233149u  /* hovered row */
#define COL_WHITE   0xf2f5f9u

#define MAXWINS 8
enum { WK_FILES, WK_BROWSER, WK_ABOUT };

struct win {
    int used, kind, minimized;
    int x, y, w, h;             /* cells, top-left origin, incl. frame */
    char title[28];
    /* Files state */
    char path[96];
    struct ramfs_node *viewing; /* 0: directory listing */
};

static struct win wins[MAXWINS];
static int nwins;

static int menu_open = -1;      /* start menu: hovered item, -2 closed */
static bool menu_visible;

/* ── small helpers ──────────────────────────────────────────────────── */

static int cell_x(void) { return (int)mouse_x() / 8; }
static int cell_y(void) { return (int)mouse_y() / 16; }

static void put(int col, int row, char ch, uint32_t fg, uint32_t bg)
{
    console_draw_cell((unsigned)col, (unsigned)row, ch, fg, bg);
}

static void put_str(int col, int row, const char *s, uint32_t fg,
                    uint32_t bg, int maxlen)
{
    for (int i = 0; i < maxlen && s[i]; i++)
        put(col + i, row, s[i], fg, bg);
}

/* ── wallpaper ──────────────────────────────────────────────────────── */

/* the boot module wallpaper.bmp (same image as the Limine boot menu),
 * parsed as an uncompressed 24-bit BMP */
static const uint8_t *wall_pix;
static int wall_w, wall_h, wall_pitch;
static int wall_flip; /* positive BMP height = scanlines bottom-up */

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

extern const void *boot_module(const char *needle, size_t *size);

static void wallpaper_init(void)
{
    size_t sz = 0;
    const uint8_t *m = boot_module("wallpaper.bmp", &sz);
    if (!m || sz < 54 || m[0] != 'B' || m[1] != 'M')
        return;
    uint32_t off = le32(m + 10);
    int32_t w = (int32_t)le32(m + 18);
    int32_t h = (int32_t)le32(m + 22);
    uint16_t bpp = le16(m + 28);
    if (bpp != 24 || w <= 0 || h == 0)
        return;
    int ah = h < 0 ? -h : h;
    size_t pitch = ((size_t)w * 3 + 3) & ~(size_t)3;
    if ((size_t)off + pitch * (size_t)ah > sz)
        return;
    wall_pix = m + off;
    wall_w = w;
    wall_h = ah;
    wall_pitch = (int)pitch;
    wall_flip = h > 0;
}

static void draw_wallpaper(void)
{
    if (wall_pix) {
        console_draw_image24(wall_pix, wall_w, wall_h, wall_pitch,
                             wall_flip);
        return;
    }
    /* fallback: the old diagonal two-tone pattern */
    unsigned cols = console_cols(), rows = console_rows();
    static const uint32_t band[4] = {
        COL_WALL0, COL_WALL1, COL_WALL2, COL_WALL3
    };
    for (unsigned r = 0; r < rows; r++)
        for (unsigned c = 0; c < cols; c++)
            console_draw_cell(c, r, ' ', COL_TEXT,
                              band[(((c + r) / 2) % 4)]);
}

/* ── windows ────────────────────────────────────────────────────────── */

static struct win *win_new(int kind, const char *title)
{
    if (nwins >= MAXWINS)
        return NULL;
    struct win *w = &wins[nwins++];
    memset(w, 0, sizeof(*w));
    w->used = 1;
    w->kind = kind;
    w->minimized = 0;
    w->w = 58;
    w->h = 26;
    /* cascade */
    w->x = 8 + (nwins - 1) * 3;
    w->y = 2 + (nwins - 1) * 2;
    unsigned rows = console_rows();
    if (w->y + w->h > (int)rows - 2)
        w->y = (int)rows - 2 - w->h;
    snprintf(w->title, sizeof(w->title), "%s", title);
    if (kind == WK_FILES)
        strcpy(w->path, "/");
    return w;
}

static void win_close(int i)
{
    memmove(&wins[i], &wins[i + 1], (size_t)(nwins - i - 1) * sizeof(wins[0]));
    nwins--;
}

static void win_raise(int i)
{
    if (i == nwins - 1)
        return;
    struct win top = wins[i];
    memmove(&wins[i], &wins[i + 1], (size_t)(nwins - i - 1) * sizeof(wins[0]));
    wins[nwins - 1] = top;
}

static int win_focused(void)
{
    return nwins ? nwins - 1 : -1;
}

static int win_at(int cx, int cy)
{
    for (int i = nwins - 1; i >= 0; i--) {
        struct win *w = &wins[i];
        if (w->minimized)
            continue;
        if (cx >= w->x && cx < w->x + w->w &&
            cy >= w->y && cy < w->y + w->h)
            return i;
    }
    return -1;
}

static void frame(int x0, int y0, int x1, int y1, uint32_t col)
{
    console_fill(x0, y0, x1, y1, col);
}

static void draw_window(int i)
{
    struct win *w = &wins[i];
    bool focus = i == win_focused();
    int x0 = w->x, y0 = w->y, x1 = w->x + w->w - 1, y1 = w->y + w->h - 1;
    unsigned cols = console_cols(), rows = console_rows();
    if (x1 >= (int)cols)
        x1 = (int)cols - 1;
    if (y1 >= (int)rows)
        y1 = (int)rows - 1;

    /* frame + body */
    frame(x0, y0, x1, y1, COL_FRAME);
    if (y1 > y0 && x1 > x0)
        console_fill(x0 + 1, y0 + 1, x1 - 1, y1 - 1, COL_PANEL);

    /* title bar */
    console_fill(x0, y0, x1, y0, focus ? COL_TITLE : COL_TITLE_D);
    put_str(x0 + 2, y0, w->title, focus ? COL_WHITE : COL_DIM,
            focus ? COL_TITLE : COL_TITLE_D, w->w - 6);

    /* close box [x] */
    if (x1 - 3 > x0 + 1) {
        put(x1 - 3, y0, '[', COL_TEXT, focus ? COL_TITLE : COL_TITLE_D);
        put(x1 - 2, y0, 'x', COL_TEXT, focus ? COL_TITLE : COL_TITLE_D);
        put(x1 - 1, y0, ']', COL_TEXT, focus ? COL_TITLE : COL_TITLE_D);
    }

    int bx0 = x0 + 1, bx1 = x1 - 1; /* body interior */

    if (w->kind == WK_FILES) {
        /* toolbar: [↑] + path */
        put(bx0 + 1, y0 + 1, ' ', COL_TEXT, COL_BTN);
        put(bx0 + 2, y0 + 1, '^', COL_TEXT, COL_BTN);
        put(bx0 + 3, y0 + 1, ' ', COL_TEXT, COL_BTN);
        put_str(bx0 + 5, y0 + 1, w->path, COL_DIM, COL_PANEL,
                bx1 - (bx0 + 5));

        /* listing or file view */
        struct ramfs_node *kids[32];
        size_t n = 0;
        if (!w->viewing) {
            struct ramfs_node *d =
                ramfs_resolve_at(ramfs_root(), w->path);
            if (d)
                n = ramfs_children(d, kids, 32);
        }
        int row = y0 + 3;
        int max_row = y1 - 2;
        if (w->viewing) {
            const char *data = ramfs_data(w->viewing);
            int pos = 0;
            while (row <= max_row && data[pos]) {
                char line[80];
                size_t len = 0;
                while (data[pos + len] && data[pos + len] != '\n' &&
                       len < sizeof(line) - 1)
                    len++;
                memcpy(line, data + pos, len);
                line[len] = 0;
                console_fill(bx0, row, bx1, row, COL_PANEL);
                put_str(bx0 + 1, row, line, COL_TEXT, COL_PANEL,
                        bx1 - bx0 - 1);
                pos += (int)len;
                if (data[pos] == '\n')
                    pos++;
                row++;
            }
        } else {
            for (size_t k = 0; k < n && row <= max_row; k++, row++) {
                bool dir = ramfs_is_dir(kids[k]);
                console_fill(bx0, row, bx1, row, COL_PANEL);
                char line[80];
                snprintf(line, sizeof(line), " %s%s",
                         ramfs_node_name(kids[k]), dir ? "/" : "");
                put_str(bx0, row, line, dir ? COL_DIR : COL_TEXT,
                        COL_PANEL, bx1 - bx0);
            }
        }
        for (; row <= max_row; row++)
            console_fill(bx0, row, bx1, row, COL_PANEL);

        /* status row */
        char status[64];
        if (w->viewing)
            snprintf(status, sizeof(status), "%s - click ^ to go back",
                     ramfs_node_name(w->viewing));
        else
            snprintf(status, sizeof(status), "%llu items - KelpOS Files",
                     (unsigned long long)n);
        console_fill(bx0, y1 - 1, bx1, y1 - 1, COL_FRAME);
        put_str(bx0 + 1, y1 - 1, status, COL_DIM, COL_FRAME,
                bx1 - bx0 - 1);
    } else {
        const char *const *lines;
        int n;
        if (w->kind == WK_ABOUT) {
            static const char *const about[] = {
                "",
                "   KelpOS " KELPOS_VERSION " (x86_64)",
                "",
                "   A free (GPL-3.0) Unix-like hobby kernel.",
                "   Boot: Limine Boot Protocol",
                "   Language: C, optional Rust",
                "   Desktop: mouse-driven window manager.",
                "   Developed with AI assistance.",
                "",
                "   (c) 2025-2026 KelpOS contributors",
            };
            lines = about;
            n = 10;
        } else {
            static const char *const browser[] = {
                "",
                "   KelpOS Web Browser",
                "",
                "   No network stack yet — the browser ships",
                "   once TCP/IP + sockets land (roadmap M5).",
                "",
                "   Until then: the sea is calm.",
            };
            lines = browser;
            n = 7;
        }
        int row = y0 + 2;
        for (int k = 0; k < n && row < y1 - 1; k++, row++) {
            console_fill(bx0, row, bx1, row, COL_PANEL);
            put_str(bx0 + 1, row, lines[k], COL_TEXT, COL_PANEL,
                    bx1 - bx0 - 1);
        }
    }
}

/* ── taskbar + start menu ───────────────────────────────────────────── */

static void draw_taskbar(void)
{
    unsigned cols = console_cols(), rows = console_rows();
    int bar = (int)rows - 1;
    console_fill(0, bar, (int)cols - 1, bar, COL_TASKBAR);

    /* start button */
    bool hov = cell_y() == bar && cell_x() < 9;
    console_fill(0, bar, 8, bar, hov ? COL_START_H : COL_START);
    put_str(1, bar, "KelpOS", COL_WHITE, hov ? COL_START_H : COL_START, 8);

    /* one button per open window */
    int bx = 10;
    for (int i = 0; i < nwins && bx < (int)cols - 12; i++) {
        struct win *w = &wins[i];
        int bw = 18;
        bool h = cell_y() == bar && cell_x() >= bx && cell_x() < bx + bw;
        uint32_t bg = i == win_focused() ? COL_BTN_HOV
                                         : (h ? COL_BTN_HOV : COL_BTN);
        console_fill(bx, bar, bx + bw - 1, bar, bg);
        put(bx, bar, w->minimized ? '+' : ' ', COL_TEXT, bg);
        put_str(bx + 1, bar, w->title, COL_TEXT, bg, bw - 2);
        bx += bw;
    }

    /* clock (CMOS RTC) */
    struct rtc_time t;
    rtc_get(&t);
    char clock[24];
    snprintf(clock, sizeof(clock), "%02d:%02d:%02d", t.hour, t.min, t.sec);
    size_t cl = strlen(clock);
    put_str((int)cols - (int)cl - 1, bar, clock, COL_DIM, COL_TASKBAR, cl);
}

static const char *const menu_items[] = {
    "Terminal", "Files", "Browser", "About", "Reboot", "Shutdown"
};
#define NMENU 6

static void draw_menu(void)
{
    if (!menu_visible)
        return;
    int x0 = 1, y1 = (int)console_rows() - 2;
    int y0 = y1 - (NMENU + 1);
    frame(x0, y0, x0 + 13, y1, COL_FRAME);
    for (int k = 0; k < NMENU; k++) {
        int row = y0 + 1 + k;
        uint32_t bg = k == menu_open ? COL_SEL : COL_PANEL;
        console_fill(x0 + 1, row, x0 + 12, row, bg);
        put_str(x0 + 2, row, menu_items[k], COL_TEXT, bg, 11);
    }
}

static void draw_screen(void)
{
    draw_wallpaper();
    for (int i = 0; i < nwins; i++)
        if (!wins[i].minimized)
            draw_window(i);
    draw_menu();
    draw_taskbar();
    console_cursor_repaint();
}

/* ── actions ────────────────────────────────────────────────────────── */

static void strip_last(char *path)
{
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] != '/')
        len--;
    if (len > 1)
        len--; /* keep the '/' */
    path[len] = 0;
    if (!path[0])
        strcpy(path, "/");
}

static void open_files(void)
{
    /* raise an existing Files window if there is one */
    for (int i = 0; i < nwins; i++)
        if (wins[i].kind == WK_FILES) {
            wins[i].minimized = 0;
            win_raise(i);
            return;
        }
    win_new(WK_FILES, "Files");
}

static void menu_action(int k)
{
    switch (k) {
    case 0: /* Terminal */
        console_cursor_hide();
        console_set_colors(KELP_FG, KELP_BG);
        console_clear();
        shell_run(); /* noreturn */
        break;
    case 1:
        open_files();
        break;
    case 2:
        for (int i = 0; i < nwins; i++)
            if (wins[i].kind == WK_BROWSER) {
                wins[i].minimized = 0;
                win_raise(i);
                goto done;
            }
        win_new(WK_BROWSER, "Browser");
        break;
    case 3:
        for (int i = 0; i < nwins; i++)
            if (wins[i].kind == WK_ABOUT) {
                wins[i].minimized = 0;
                win_raise(i);
                goto done;
            }
        win_new(WK_ABOUT, "About KelpOS");
        break;
    case 4:
        console_cursor_hide();
        cmd_reboot(0, 0); /* noreturn */
        break;
    case 5:
        console_cursor_hide();
        cmd_shutdown(0, 0); /* noreturn */
        break;
    }
done:
    menu_visible = false;
}

/* ── event handling ─────────────────────────────────────────────────── */

static int  win_focused(void);
static void start_drag(int i, int cx, int cy);

/* set by every state change that needs a full repaint (open/close/
 * move/raise windows, menu changes). The main loop repaints only when
 * this is set, so plain cursor movement never redraws the screen —
 * that is what made the desktop flicker. */
static bool screen_dirty;

static void handle_click(void)
{
    int cx = cell_x(), cy = cell_y();
    unsigned cols = console_cols(), rows = console_rows();
    int bar = (int)rows - 1;

    /* start menu open: items or outside-click closes */
    if (menu_visible) {
        int x0 = 1, y0 = (int)rows - 2 - (NMENU + 1);
        if (cx >= x0 + 1 && cx <= x0 + 12 &&
            cy > y0 && cy <= y0 + NMENU)
            menu_action(cy - y0 - 1);
        menu_visible = false;
        screen_dirty = true; /* the menu must be erased */
        return;
    }
    /* taskbar */
    if (cy == bar) {
        if (cx < 9) {
            menu_visible = true;
            menu_open = -1;
            screen_dirty = true;
        } else {
            int bx = 10;
            for (int i = 0; i < nwins && bx < (int)cols - 12; i++) {
                if (cx >= bx && cx < bx + 18) {
                    wins[i].minimized = !wins[i].minimized;
                    win_raise(i);
                    screen_dirty = true;
                    return;
                }
                bx += 18;
            }
        }
        return;
    }

    /* windows, topmost first */
    int i = win_at(cx, cy);
    if (i < 0)
        return;
    win_raise(i);
    screen_dirty = true; /* raise/focus changes the z-order */
    struct win *w = &wins[i];
    int x1 = w->x + w->w - 1, y0 = w->y;

    if (cy == y0) {
        if (cx >= x1 - 3) { /* close box */
            win_close(i);
            screen_dirty = true;
            return;
        }
        start_drag(i, cx, cy);
        return;
    }

    if (w->kind != WK_FILES)
        return;
    int bx0 = w->x + 1, bx1 = x1 - 1;

    /* toolbar ^ button */
    if (cy == y0 + 1 && cx >= bx0 + 1 && cx <= bx0 + 3) {
        if (w->viewing)
            w->viewing = 0;
        else
            strip_last(w->path);
        screen_dirty = true;
        return;
    }

    /* body rows */
    if (cy < y0 + 3 || cy > y0 + w->h - 3 || cx < bx0 || cx > bx1)
        return;

    if (w->viewing) {
        w->viewing = 0;
        screen_dirty = true;
        return;
    }
    struct ramfs_node *d = ramfs_resolve_at(ramfs_root(), w->path);
    if (!d)
        return;
    struct ramfs_node *kids[32];
    size_t n = ramfs_children(d, kids, 32);
    int idx = cy - (y0 + 3);
    if (idx < 0 || idx >= (int)n)
        return;
    if (ramfs_is_dir(kids[idx])) {
        char newpath[96];
        snprintf(newpath, sizeof(newpath), "%s%s%s",
                 w->path, strcmp(w->path, "/") ? "/" : "",
                 ramfs_node_name(kids[idx]));
        snprintf(w->path, sizeof(w->path), "%s", newpath);
        screen_dirty = true;
    } else {
        w->viewing = kids[idx];
        screen_dirty = true;
    }
}

/* drag state */
static int drag = -1, drag_dx, drag_dy;

static void start_drag(int i, int cx, int cy)
{
    drag = i;
    drag_dx = cx - wins[i].x;
    drag_dy = cy - wins[i].y;
}

/* moved: whether the mouse moved in this loop iteration (the moved
 * flag is already consumed by the main loop, so it is passed in) */
static void poll_drag(bool moved)
{
    if (drag < 0)
        return;
    if (!(mouse_buttons() & 0x01)) {
        drag = -1;
        return;
    }
    if (!moved)
        return;
    struct win *w = &wins[drag];
    unsigned cols = console_cols(), rows = console_rows();
    int nx = cell_x() - drag_dx;
    int ny = cell_y() - drag_dy;    if (nx < 0)
        nx = 0;
    if (ny < 0)
        ny = 0;
    if (nx > (int)cols - 6)
        nx = (int)cols - 6;
    if (ny > (int)rows - 3)
        ny = (int)rows - 3;
    if (nx != w->x || ny != w->y) {
        w->x = nx;
        w->y = ny;
        screen_dirty = true;
    }
}

/* ── main loop ──────────────────────────────────────────────────────── */

/* what the cursor currently points at on the taskbar/menu, so the
 * hover highlight is only repainted when it actually changes:
 * -1 nothing, 0 start button, 1..n window buttons, 100+k menu item */
static int hover_sig(void)
{
    int cx = cell_x(), cy = cell_y();
    unsigned cols = console_cols(), rows = console_rows();
    int bar = (int)rows - 1;
    if (menu_visible) {
        int x0 = 1, y0 = (int)rows - 2 - (NMENU + 1);
        if (cx >= x0 + 1 && cx <= x0 + 12 &&
            cy > y0 && cy <= y0 + NMENU)
            return 100 + (cy - y0 - 1);
        return -1;
    }
    if (cy == bar) {
        if (cx < 9)
            return 0;
        int bx = 10;
        for (int i = 0; i < nwins && bx < (int)cols - 12; i++) {
            if (cx >= bx && cx < bx + 18)
                return 1 + i;
            bx += 18;
        }
    }
    return -1;
}

void desktop_run(void)
{
    if (!console_fb_active()) {
        kprintf("Desktop needs a framebuffer — starting shell instead.\n");
        shell_run();
    }

    nwins = 0;
    menu_visible = false;
    drag = -1;
    wallpaper_init();
    draw_screen();
    console_cursor_move(mouse_x(), mouse_y()); /* show the cursor */

    unsigned long long last_sec = (unsigned long long)-1;
    int last_hover = hover_sig();
    for (;;) {
        screen_dirty = false;

        /* read both edges ONCE: consuming them in a short-circuit OR
         * would drop the click whenever the mouse did not move */
        int moved = mouse_moved();
        int clicked = mouse_left_click();
        if (moved || clicked) {
            console_cursor_move(mouse_x(), mouse_y());
            poll_drag(moved != 0);
            if (clicked)
                handle_click();
            /* hovering a menu item selects it */
            if (menu_visible) {
                int hov = hover_sig();
                if (hov >= 100 && hov - 100 != menu_open) {
                    menu_open = hov - 100;
                    screen_dirty = true;
                }
            }
        }

        int c = kbd_poll();
        if (c != -1) {
            if (menu_visible) {
                if (c == KBD_UP && menu_open > 0) {
                    menu_open--;
                    screen_dirty = true;
                } else if (c == KBD_DOWN && menu_open < NMENU - 1) {
                    menu_open++;
                    screen_dirty = true;
                } else if (c == '\n' && menu_open >= 0) {
                    menu_action(menu_open);
                    screen_dirty = true;
                } else if (c == KBD_ESC) {
                    menu_visible = false;
                    screen_dirty = true;
                }
            } else {
                if (c == KBD_ESC && nwins) {
                    win_close(win_focused());
                    screen_dirty = true;
                } else if (c == '\n') {
                    menu_visible = true;
                    menu_open = 0;
                    screen_dirty = true;
                }
            }
        }

        /* hover highlight: only the taskbar, only when it changes */
        int hover = hover_sig();
        if (hover != last_hover) {
            last_hover = hover;
            draw_taskbar();
            console_cursor_repaint();
        }

        /* clock tick: redraw the taskbar when the second changes */
        unsigned long long sec = timer_ms() / 1000;
        if (sec != last_sec) {
            last_sec = sec;
            draw_taskbar();
            console_cursor_repaint();
        }

        /* full repaint only on real state changes */
        if (screen_dirty)
            draw_screen();

        __asm__ volatile("hlt");
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
