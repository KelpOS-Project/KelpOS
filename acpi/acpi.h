#ifndef KELPOS_ACPI_H
#define KELPOS_ACPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct acpi_root_info {
    uint64_t address;
    bool xsdt;
    uint8_t revision;
};

bool acpi_find_rsdp(const volatile uint8_t *memory, size_t length,
    struct acpi_root_info *root);
bool acpi_initialize(struct acpi_root_info *root);

#endif