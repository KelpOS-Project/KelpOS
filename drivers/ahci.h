#ifndef KELP_AHCI_H
#define KELP_AHCI_H

/* AHCI (SATA) host controller driver. Probes the PCI bus for an AHCI
 * controller, brings up every attached ATA disk and registers it with
 * the block layer as sda, sdb, ... (Unix-style names). */

int ahci_init(void);

/* Flush the write caches of every AHCI disk. Used by `sync`. */
int ahci_flush_all(void);

/* True if at least one AHCI disk was registered. */
int ahci_present(void);

#endif
