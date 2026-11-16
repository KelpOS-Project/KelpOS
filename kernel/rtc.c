/* kernel/rtc.c — CMOS real-time clock readout for `date`. */
#include <stdint.h>
#include "io.h"
#include "rtc.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t cmos_read(uint8_t reg)
{
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static int bcd(uint8_t v, uint8_t regb)
{
    if (regb & 0x04)
        return v;              /* binary mode */
    return (v & 0x0F) + (v >> 4) * 10;
}

void rtc_get(struct rtc_time *t)
{
    /* wait for an update-in-progress cycle to finish */
    for (int i = 0; i < 1000000 && (cmos_read(0x0A) & 0x80); i++)
        ;

    uint8_t regb = cmos_read(0x0B);
    uint8_t hour = cmos_read(0x04);

    t->sec = bcd(cmos_read(0x00), regb);
    t->min = bcd(cmos_read(0x02), regb);
    if (regb & 0x02 && !(regb & 0x04)) { /* 12-hour BCD */
        int h = (hour & 0x0F) + (hour >> 4) * 10;
        if (hour & 0x80)
            h = h == 12 ? 12 : h + 12;
        else
            h = h == 12 ? 0 : h;
        t->hour = h;
    } else if (!(regb & 0x02)) { /* 12-hour binary */
        t->hour = hour & 0x7F;
        if (hour & 0x80 && t->hour != 12)
            t->hour += 12;
        if (hour & 0x80 && t->hour == 12)
            t->hour = 12;
    } else {
        t->hour = bcd(hour, regb);
    }
    t->day = bcd(cmos_read(0x07), regb);
    t->month = bcd(cmos_read(0x08), regb);
    t->year = bcd(cmos_read(0x09), regb) + (bcd(cmos_read(0x32), regb) * 100);
}
