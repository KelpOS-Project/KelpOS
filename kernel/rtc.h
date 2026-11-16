#ifndef KELP_RTC_H
#define KELP_RTC_H

/* CMOS RTC (ports 0x70/0x71) — wall-clock time for the `date` command. */

struct rtc_time {
    int sec, min, hour;
    int day, month;
    int year; /* full year, e.g. 2026 */
};

void rtc_get(struct rtc_time *t);

#endif
