#ifndef KELP_TIMER_H
#define KELP_TIMER_H

#include <stdint.h>

void timer_init(unsigned hz);
void timer_tick(void);
uint64_t timer_ms(void);

#endif
