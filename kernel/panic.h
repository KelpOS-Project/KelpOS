#ifndef KELPOS_KERNEL_PANIC_H
#define KELPOS_KERNEL_PANIC_H

#include <stdint.h>

_Noreturn void kernel_panic(const char *reason);
_Noreturn void kernel_panic_exception(const char *reason, uint64_t vector,
	uint64_t error_code);

#endif