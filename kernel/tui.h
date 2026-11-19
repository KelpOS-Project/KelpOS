#ifndef KELP_TUI_H
#define KELP_TUI_H

#include <stdbool.h>
#include <stdint.h>

/* Small ncurses-style widget layer for full-screen text UIs on the
 * framebuffer terminal (the look of Linux `make menuconfig`).
 *
 * tui_begin() clears the screen and suspends normal kprintf drawing on
 * the framebuffer (serial output keeps working, so logs stay usable);
 * tui_end() restores a clean shell screen. The layer only works when
 * console_fb_active() is true and the terminal is big enough: callers
 * must fall back to plain line output otherwise (see kelpinstall). */

struct tui_item {
    const char *label;    /* NULL draws a separator line */
    const char *value;    /* right-aligned value column, or NULL */
    const char *hint;     /* one-line help under the list, or NULL */
    int id;               /* caller-defined */
    bool disabled;        /* drawn dimmed, skipped by the cursor */
};

void tui_begin(void);
void tui_end(void);

/* True when a TUI can be shown (framebuffer terminal, sane size). */
bool tui_available(void);

/* Menu: returns the chosen index, or -1 on Esc. `sel` is the initial
 * selection (clamped to an enabled entry); `footer` may be NULL. */
int tui_menu(const char *title, const char *subtitle,
             const struct tui_item *items, int count, int sel,
             const char *footer);

/* Text page; returns the key that dismissed it (Enter, Esc or Space). */
int tui_page(const char *title, const char *heading,
             const char *const *lines, int count, const char *footer);

/* Modal confirmation with two buttons. Returns true for the first
 * (left) button, false for the second one or Esc. */
bool tui_confirm(const char *title, const char *heading,
                 const char *const *lines, int count,
                 const char *ok, const char *cancel);

/* Progress screen. Cheap enough to redraw once per percent. */
void tui_progress(const char *title, const char *stage, unsigned pct,
                  const char *detail);

/* Low-level helpers (screen cell coordinates, clipped). */
void tui_put(int col, int row, const char *s, uint32_t fg, uint32_t bg);
void tui_fill(int col0, int row0, int col1, int row1, uint32_t bg);
int  tui_cols(void);
int  tui_rows(void);

#endif
