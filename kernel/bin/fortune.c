/* kernel/bin/fortune.c — package "fortune": random quotes about
 * free software and kernels. */
#include "commands.h"

#ifdef KELPOS_PKG_FORTUNE
#include <stdint.h>
#include <stdio.h>
#include "timer.h"

static const char *const quotes[] = {
    "Free software is a matter of freedom, not price.  -- RMS",
    "Talk is cheap. Show me the code.  -- Linus Torvalds",
    "Given enough eyeballs, all bugs are shallow.  -- Linus' Law",
    "Those who don't understand Unix are doomed to reinvent it, poorly.",
    "A kernel without a filesystem is just a very stubborn loop.",
    "Release early, release often.",
    "The best documentation is source code you can read.",
    "Entropy flows downhill — even in a kelp forest.",
    "perpetual debugging is the true meaning of life",
    "Stand on the shoulders of giants: read the GPL.",
};

static int cmd_fortune(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint64_t t = timer_ms();
    /* cheap mixing so consecutive calls differ even at boot */
    uint32_t r = (uint32_t)(t ^ (t >> 17) ^ (t >> 7));
    kprintf("%s\n", quotes[r % (sizeof(quotes) / sizeof(quotes[0]))]);
    return 0;
}

const struct builtin fortune_cmds[] = {
    { "fortune", cmd_fortune, "print a (hopefully wise) quote" },
};
#endif
