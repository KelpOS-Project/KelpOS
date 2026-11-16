/* kernel/bin/date.c — core: wall-clock date from the CMOS RTC */
#include "commands.h"
#include "rtc.h"
#include <stdio.h>

int cmd_date(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct rtc_time t;
    rtc_get(&t);
    static const char *const wdays[] = {
        0, "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    /* day of week (Zeller-ish, weekday of CMOS 0x06 not always sane) */
    int q = t.day, m = t.month, y = t.year;
    if (m < 3) {
        m += 12;
        y--;
    }
    int k = y % 100, j = y / 100;
    int wd = (q + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    kprintf("%s %04d-%02d-%02d %02d:%02d:%02d UTC\n",
            wdays[wd + 1], t.year, t.month, t.day,
            t.hour, t.min, t.sec);
    return 0;
}
