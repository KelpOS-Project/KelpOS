#include "commands.h"
#include "../panic.h"

void cmd_syscrash(const char *args)
{
    (void)args;
    kernel_panic("Triggered System Crash");
}