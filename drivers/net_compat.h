#ifndef NET_COMPAT_H
#define NET_COMPAT_H

#include <stdint.h>
#include <stddef.h>

/* Low-Level Port I/O für x86/x86_64 Hardware-Zugriffe */
static inline uint8_t kelp_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void kelp_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Mappe BoredOS-Makros direkt auf die KelpOS-Primitiven */
#define bored_inb(port)       kelp_inb(port)
#define bored_outb(port, val) kelp_outb(port, val)

/* Initialisierung für den Kernel */
void kelp_net_subsystem_init(void);

#endif /* NET_COMPAT_H */