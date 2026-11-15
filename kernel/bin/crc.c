/* kernel/bin/crc.c — package "crc": CRC32 checksum demo.
 * Uses the optional Rust function krust_crc32 (rust/krust); falls back
 * to a plain C implementation when the Rust library is not linked. */
#include "commands.h"

#ifdef KELPOS_PKG_CRC
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern uint32_t krust_crc32(const uint8_t *data, size_t len) __attribute__((weak));

static uint32_t crc32_c(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1)));
    }
    return ~crc;
}

static int cmd_crc(int argc, char **argv)
{
    const char *s = argc > 1 ? argv[1] : "KelpOS";
    uint32_t crc;
    const char *src;

    if (krust_crc32) {
        crc = krust_crc32((const uint8_t *)s, strlen(s));
        src = "rust";
    } else {
        crc = crc32_c((const uint8_t *)s, strlen(s));
        src = "c-fallback";
    }
    kprintf("CRC32(\"%s\") = %08x [%s]\n", s, crc, src);
    return 0;
}

const struct builtin crc_cmds[] = {
    { "crc", cmd_crc, "CRC32 checksum of an argument (Rust demo)" },
};
#endif
