#ifndef KELP_SHELL_H
#define KELP_SHELL_H

#include <stdbool.h>

/* Runs the interactive shell. Never returns (use the `desktop` command
 * to switch into the graphical desktop). */
void shell_run(void) __attribute__((noreturn));

/* current working directory of the shell (static "/" or deeper) */
const char *shell_cwd(void);
/* change directory; returns 0 on success, -1 if the path is not a dir */
int  shell_chdir(const char *path);

#endif
