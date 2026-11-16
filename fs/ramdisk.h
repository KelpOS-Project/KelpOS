#ifndef KELP_FS_RAMDISK_H
#define KELP_FS_RAMDISK_H

#include <stddef.h>
#include <stdint.h>

/* Memory-backed block device: typically backed by a file loaded by the
 * bootloader as a module (e.g. a FAT disk image shipped in the ISO).
 * Writes land in RAM and are lost on reboot — read-mostly demo device. */

int ramdisk_register(const char *name, uintptr_t addr, size_t size);

#endif
