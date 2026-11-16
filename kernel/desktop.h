#ifndef KELP_DESKTOP_H
#define KELP_DESKTOP_H

/* Basic graphical desktop (framebuffer, keyboard-driven).
 * Never returns; the Terminal app drops into the shell, from which
 * `desktop` re-enters. Falls back to the shell without a framebuffer. */
void desktop_run(void) __attribute__((noreturn));

#endif
