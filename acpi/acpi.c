#include "acpi.h"

enum {
    BIOS_SCAN_START = 0xE0000,
    BIOS_SCAN_LENGTH = 0x20000,
    RSDP_V1_LENGTH = 20,
    RSDP_V2_MIN_LENGTH = 36,
    MAX_RSDP_LENGTH = 4096,
};

static uint64_t read_u64(const volatile uint8_t *bytes)
{
    uint64_t value = 0;
    for (unsigned int index = 0; index < 8; index++) {
        value |= (uint64_t)bytes[index] << (index * 8);
    }
    return value;
}

static uint32_t read_u32(const volatile uint8_t *bytes)
{
    uint32_t value = 0;
    for (unsigned int index = 0; index < 4; index++) {
        value |= (uint32_t)bytes[index] << (index * 8);
    }
    return value;
}

static bool checksum_valid(const volatile uint8_t *bytes, size_t length)
{
    uint8_t sum = 0;
    for (size_t index = 0; index < length; index++) {
        sum = (uint8_t)(sum + bytes[index]);
    }
    return sum == 0;
}

bool acpi_find_rsdp(const volatile uint8_t *memory, size_t length,
    struct acpi_root_info *root)
{
    static const char signature[] = "RSD PTR ";
    if (memory == NULL || root == NULL || length < RSDP_V1_LENGTH) {
        return false;
    }

    for (size_t offset = 0; offset <= length - RSDP_V1_LENGTH; offset += 16) {
        bool matches = true;
        for (size_t index = 0; index < sizeof(signature) - 1; index++) {
            matches &= memory[offset + index] == (uint8_t)signature[index];
        }
        if (!matches || !checksum_valid(memory + offset, RSDP_V1_LENGTH)) {
            continue;
        }

        const uint8_t revision = memory[offset + 15];
        uint64_t address = read_u32(memory + offset + 16);
        bool xsdt = false;
        if (revision >= 2) {
            if (length - offset < RSDP_V2_MIN_LENGTH) {
                continue;
            }
            const uint32_t rsdp_length = read_u32(memory + offset + 20);
            if (rsdp_length < RSDP_V2_MIN_LENGTH || rsdp_length > MAX_RSDP_LENGTH
                    || rsdp_length > length - offset
                    || !checksum_valid(memory + offset, rsdp_length)) {
                continue;
            }
            const uint64_t xsdt_address = read_u64(memory + offset + 24);
            if (xsdt_address != 0) {
                address = xsdt_address;
                xsdt = true;
            }
        }
        if (address == 0) {
            continue;
        }
        root->address = address;
        root->xsdt = xsdt;
        root->revision = revision;
        return true;
    }
    return false;
}

bool acpi_initialize(struct acpi_root_info *root)
{
    return acpi_find_rsdp((const volatile uint8_t *)(uintptr_t)BIOS_SCAN_START,
        BIOS_SCAN_LENGTH, root);
}