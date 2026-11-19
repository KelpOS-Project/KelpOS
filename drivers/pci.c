/* drivers/pci.c — PCI configuration space (I/O ports 0xCF8/0xCFC). */
#include <stdint.h>
#include "io.h"
#include "pci.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static uint32_t cfg_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off)
{
    return 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
           ((uint32_t)func << 8) | (uint32_t)(off & 0xFC);
}

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off)
{
    outl(PCI_CONFIG_ADDR, cfg_addr(bus, slot, func, off));
    return inl(PCI_CONFIG_DATA);
}

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off,
                 uint32_t val)
{
    outl(PCI_CONFIG_ADDR, cfg_addr(bus, slot, func, off));
    outl(PCI_CONFIG_DATA, val);
}

int pci_find_class(uint8_t class_code, uint8_t subclass, int prog_if,
                   uint8_t *bus, uint8_t *slot, uint8_t *func)
{
    for (int b = 0; b < 256; b++) {
        for (int s = 0; s < 32; s++) {
            for (int f = 0; f < 8; f++) {
                uint32_t id = pci_read32((uint8_t)b, (uint8_t)s, (uint8_t)f, 0);
                if ((id & 0xFFFF) == 0xFFFF)
                    continue;
                uint32_t cc = pci_read32((uint8_t)b, (uint8_t)s, (uint8_t)f,
                                         0x08);
                if ((int)((cc >> 24) & 0xFF) != class_code)
                    continue;
                if ((int)((cc >> 16) & 0xFF) != subclass)
                    continue;
                if (prog_if >= 0 && (int)((cc >> 8) & 0xFF) != prog_if)
                    continue;
                if (bus)
                    *bus = (uint8_t)b;
                if (slot)
                    *slot = (uint8_t)s;
                if (func)
                    *func = (uint8_t)f;
                return 0;
            }
        }
    }
    return -1;
}

uintptr_t pci_bar_mmio(uint8_t bus, uint8_t slot, uint8_t func, int bar)
{
    uint32_t lo = pci_read32(bus, slot, func, (uint8_t)(0x10 + bar * 4));
    if (lo & 0x1)
        return 0; /* I/O BAR */
    uint32_t hi = 0;
    if ((lo & 0x6) == 0x4) /* 64-bit BAR */
        hi = pci_read32(bus, slot, func, (uint8_t)(0x10 + bar * 4 + 4));
    uintptr_t addr = (uintptr_t)(lo & ~0xFu) | ((uintptr_t)hi << 32);
    return addr;
}
