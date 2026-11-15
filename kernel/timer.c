/* kernel/timer.c — PIT channel 0 as the system ticker */
#include <stdint.h>
#include "io.h"
#include "timer.h"

static volatile uint64_t ticks;
static uint64_t ms_per_tick = 1;

void timer_init(unsigned hz)
{
    unsigned divisor = 1193182 / hz;
    outb(0x43, 0x36);              /* channel 0, lo/hi bytes, mode 3 */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    ms_per_tick = 1000 / hz;
}

void timer_tick(void)
{
    ticks++;
}

uint64_t timer_ms(void)
{
    return ticks * ms_per_tick;
}
