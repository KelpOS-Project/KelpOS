#ifndef KELP_PCI_H
#define KELP_PCI_H

#include <stdint.h>

/* Minimal PCI configuration-space access (mechanism #1). Enough for the
 * storage drivers: find a device by class, read its BARs and go. */

#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_SUBCLASS_SATA      0x06

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void     pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off,
                     uint32_t val);

/* Scan bus 0..255 for (class, subclass, prog_if); -1 if not found.
 * prog_if < 0 matches any programming interface. */
int pci_find_class(uint8_t class_code, uint8_t subclass, int prog_if,
                   uint8_t *bus, uint8_t *slot, uint8_t *func);

/* Physical address of a memory BAR (0 if it is an I/O BAR or invalid). */
uintptr_t pci_bar_mmio(uint8_t bus, uint8_t slot, uint8_t func, int bar);

#endif
