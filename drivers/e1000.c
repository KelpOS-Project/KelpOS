#include "e1000.h"

#include <stdint.h>

#include "x86_io.h"

enum {
    PCI_CONFIG_ADDRESS = 0xCF8,
    PCI_CONFIG_DATA = 0xCFC,
    PCI_COMMAND_OFFSET = 0x04,
    PCI_BAR0_OFFSET = 0x10,
    E1000_VENDOR = 0x8086,
    E1000_82540EM = 0x100E,
    E1000_82545EM = 0x100F,
    E1000_82574L = 0x10D3,
    REGISTER_CTRL = 0x0000,
    REGISTER_IMC = 0x00D8,
    REGISTER_ICR = 0x00C0,
    REGISTER_RCTL = 0x0100,
    REGISTER_TCTL = 0x0400,
    REGISTER_TIPG = 0x0410,
    REGISTER_RDBAL = 0x2800,
    REGISTER_RDBAH = 0x2804,
    REGISTER_RDLEN = 0x2808,
    REGISTER_RDH = 0x2810,
    REGISTER_RDT = 0x2818,
    REGISTER_TDBAL = 0x3800,
    REGISTER_TDBAH = 0x3804,
    REGISTER_TDLEN = 0x3808,
    REGISTER_TDH = 0x3810,
    REGISTER_TDT = 0x3818,
    DESCRIPTOR_COUNT = 16,
    BUFFER_SIZE = 2048,
    RESET_BIT = 1u << 26,
};

struct e1000_descriptor {
    uint64_t address;
    uint16_t length;
    uint8_t checksum_offset;
    uint8_t command;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed, aligned(16)));

static volatile uint32_t *registers;
static volatile struct e1000_descriptor tx_descriptors[DESCRIPTOR_COUNT] __attribute__((aligned(16)));
static volatile struct e1000_descriptor rx_descriptors[DESCRIPTOR_COUNT] __attribute__((aligned(16)));
static uint8_t tx_buffers[DESCRIPTOR_COUNT][BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t rx_buffers[DESCRIPTOR_COUNT][BUFFER_SIZE] __attribute__((aligned(16)));
static size_t tx_index;
static size_t rx_index;

static uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    const uint32_t address = 0x80000000u | ((uint32_t)bus << 16)
        | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);
    x86_port_write32(PCI_CONFIG_ADDRESS, address);
    return x86_port_read32(PCI_CONFIG_DATA);
}

static void pci_write(uint8_t bus, uint8_t device, uint8_t function,
    uint8_t offset, uint32_t value)
{
    const uint32_t address = 0x80000000u | ((uint32_t)bus << 16)
        | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);
    x86_port_write32(PCI_CONFIG_ADDRESS, address);
    x86_port_write32(PCI_CONFIG_DATA, value);
}

static bool find_device(uint8_t *found_bus, uint8_t *found_device,
    uint8_t *found_function)
{
    for (unsigned int bus = 0; bus < 256; bus++) {
        for (unsigned int device = 0; device < 32; device++) {
            for (unsigned int function = 0; function < 8; function++) {
                const uint32_t id = pci_read(bus, device, function, 0);
                const uint16_t vendor = (uint16_t)id;
                const uint16_t product = (uint16_t)(id >> 16);
                if (vendor == E1000_VENDOR && (product == E1000_82540EM
                        || product == E1000_82545EM || product == E1000_82574L)) {
                    *found_bus = (uint8_t)bus;
                    *found_device = (uint8_t)device;
                    *found_function = (uint8_t)function;
                    return true;
                }
                if (function == 0 && vendor == 0xFFFF) {
                    break;
                }
            }
        }
    }
    return false;
}

static void write_register(uint32_t offset, uint32_t value)
{
    registers[offset / sizeof(uint32_t)] = value;
}

bool e1000_initialize(void)
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    if (!find_device(&bus, &device, &function)) {
        return false;
    }

    const uint32_t bar = pci_read(bus, device, function, PCI_BAR0_OFFSET);
    const uint32_t base = bar & ~0x0Fu;
    if ((bar & 1) != 0 || (bar & 6) == 4 || base == 0
            || base > 0xFFFE0000u) {
        return false;
    }

    const uint32_t command = pci_read(bus, device, function, PCI_COMMAND_OFFSET);
    pci_write(bus, device, function, PCI_COMMAND_OFFSET,
        (command & 0xFFFFu) | 0x0006u);
    registers = (volatile uint32_t *)(uintptr_t)base;

    write_register(REGISTER_IMC, 0xFFFFFFFFu);
    (void)registers[REGISTER_ICR / sizeof(uint32_t)];
    write_register(REGISTER_CTRL, registers[REGISTER_CTRL / sizeof(uint32_t)] | RESET_BIT);
    for (unsigned int attempt = 0; attempt < 1000000; attempt++) {
        if ((registers[REGISTER_CTRL / sizeof(uint32_t)] & RESET_BIT) == 0) {
            break;
        }
        if (attempt == 999999) {
            registers = NULL;
            return false;
        }
    }

    for (size_t index = 0; index < DESCRIPTOR_COUNT; index++) {
        tx_descriptors[index].address = (uint64_t)(uintptr_t)tx_buffers[index];
        tx_descriptors[index].status = 1;
        rx_descriptors[index].address = (uint64_t)(uintptr_t)rx_buffers[index];
        rx_descriptors[index].status = 0;
    }

    write_register(REGISTER_TDBAL, (uint32_t)(uintptr_t)tx_descriptors);
    write_register(REGISTER_TDBAH, 0);
    write_register(REGISTER_TDLEN, sizeof(tx_descriptors));
    write_register(REGISTER_TDH, 0);
    write_register(REGISTER_TDT, 0);
    write_register(REGISTER_TIPG, 0x0060200Au);
    write_register(REGISTER_TCTL, 0x0004010Au);

    write_register(REGISTER_RDBAL, (uint32_t)(uintptr_t)rx_descriptors);
    write_register(REGISTER_RDBAH, 0);
    write_register(REGISTER_RDLEN, sizeof(rx_descriptors));
    write_register(REGISTER_RDH, 0);
    write_register(REGISTER_RDT, DESCRIPTOR_COUNT - 1);
    write_register(REGISTER_RCTL, 0x04008002u);
    tx_index = 0;
    rx_index = 0;
    return true;
}

bool e1000_is_initialized(void)
{
    return registers != NULL;
}

bool e1000_send(const void *frame, size_t length)
{
    if (registers == NULL || frame == NULL || length == 0 || length > BUFFER_SIZE) {
        return false;
    }
    volatile struct e1000_descriptor *descriptor = &tx_descriptors[tx_index];
    if ((descriptor->status & 1) == 0) {
        return false;
    }
    const uint8_t *source = frame;
    for (size_t index = 0; index < length; index++) {
        tx_buffers[tx_index][index] = source[index];
    }
    descriptor->length = (uint16_t)length;
    descriptor->checksum_offset = 0;
    descriptor->errors = 0;
    descriptor->command = 0x0B;
    descriptor->status = 0;
    tx_index = (tx_index + 1) % DESCRIPTOR_COUNT;
    write_register(REGISTER_TDT, (uint32_t)tx_index);
    return true;
}

bool e1000_receive(void *buffer, size_t capacity, size_t *length)
{
    if (registers == NULL || buffer == NULL || length == NULL) {
        return false;
    }
    volatile struct e1000_descriptor *descriptor = &rx_descriptors[rx_index];
    if ((descriptor->status & 1) == 0) {
        return false;
    }
    const size_t received = descriptor->length;
    if (descriptor->errors != 0 || received > capacity || received > BUFFER_SIZE) {
        descriptor->status = 0;
        write_register(REGISTER_RDT, (uint32_t)rx_index);
        rx_index = (rx_index + 1) % DESCRIPTOR_COUNT;
        return false;
    }
    uint8_t *destination = buffer;
    for (size_t index = 0; index < received; index++) {
        destination[index] = rx_buffers[rx_index][index];
    }
    *length = received;
    descriptor->status = 0;
    write_register(REGISTER_RDT, (uint32_t)rx_index);
    rx_index = (rx_index + 1) % DESCRIPTOR_COUNT;
    return true;
}