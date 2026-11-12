#ifndef KELPOS_DRIVERS_X86_IO_H
#define KELPOS_DRIVERS_X86_IO_H

#include <stdint.h>

uint8_t x86_port_read8(uint16_t port);
uint32_t x86_port_read32(uint16_t port);
void x86_port_write8(uint16_t port, uint8_t value);
void x86_port_write32(uint16_t port, uint32_t value);

#endif