#ifndef KELPOS_DRIVERS_E1000_H
#define KELPOS_DRIVERS_E1000_H

#include <stdbool.h>
#include <stddef.h>

bool e1000_initialize(void);
bool e1000_is_initialized(void);
bool e1000_send(const void *frame, size_t length);
bool e1000_receive(void *buffer, size_t capacity, size_t *length);

#endif