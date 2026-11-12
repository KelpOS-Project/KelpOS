#include "../acpi/acpi.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void write_u32(uint8_t *bytes, uint32_t value)
{
    for (unsigned int index = 0; index < 4; index++) {
        bytes[index] = (uint8_t)(value >> (index * 8));
    }
}

static void write_u64(uint8_t *bytes, uint64_t value)
{
    for (unsigned int index = 0; index < 8; index++) {
        bytes[index] = (uint8_t)(value >> (index * 8));
    }
}

static void set_checksum(uint8_t *bytes, size_t length, size_t checksum_offset)
{
    uint8_t sum = 0;
    bytes[checksum_offset] = 0;
    for (size_t index = 0; index < length; index++) {
        sum = (uint8_t)(sum + bytes[index]);
    }
    bytes[checksum_offset] = (uint8_t)(0 - sum);
}

int main(void)
{
    uint8_t memory[128] = { 0 };
    const size_t offset = 32;
    struct acpi_root_info root;
    const char signature[] = "RSD PTR ";
    for (size_t index = 0; index < sizeof(signature) - 1; index++) {
        memory[offset + index] = (uint8_t)signature[index];
    }
    memory[offset + 15] = 2;
    write_u32(memory + offset + 16, 0x12345000);
    write_u32(memory + offset + 20, 36);
    write_u64(memory + offset + 24, 0x12346000);
    set_checksum(memory + offset, 20, 8);
    set_checksum(memory + offset, 36, 32);

    assert(acpi_find_rsdp(memory, sizeof(memory), &root));
    assert(root.address == 0x12346000);
    assert(root.xsdt);
    assert(root.revision == 2);

    memory[offset + 8]++;
    assert(!acpi_find_rsdp(memory, sizeof(memory), &root));
    assert(!acpi_find_rsdp(memory, sizeof(memory), NULL));

    for (size_t index = 0; index < 36; index++) {
        memory[offset + index] = 0;
    }
    for (size_t index = 0; index < sizeof(signature) - 1; index++) {
        memory[offset + index] = (uint8_t)signature[index];
    }
    write_u32(memory + offset + 16, 0x12345000);
    set_checksum(memory + offset, 20, 8);
    assert(acpi_find_rsdp(memory, sizeof(memory), &root));
    assert(root.address == 0x12345000);
    assert(!root.xsdt);
    assert(root.revision == 0);
    puts("ACPI RSDP checksum and root-table tests passed");
    return 0;
}