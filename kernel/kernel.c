/* kernel/kernel.c — entry after _start (boot.S): boots via the Limine protocol */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <limine.h>
#include "version.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "shell.h"
#include "heap.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(0);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request ka_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

extern uint8_t __kernel_end[];

/* Use the largest usable memmap region outside the kernel image as heap */
static void init_heap(void)
{
    struct limine_memmap_response *m = memmap_request.response;
    if (!m || !ka_request.response)
        return;

    uintptr_t kbase = (uintptr_t)ka_request.response->physical_base;
    uintptr_t ksize = (uintptr_t)__kernel_end - 0xffffffff80000000ULL;
    if ((uintptr_t)ka_request.response->virtual_base != 0xffffffff80000000ULL)
        ksize = (uintptr_t)__kernel_end - (uintptr_t)ka_request.response->virtual_base;

    struct limine_memmap_entry *best = 0;
    for (uint64_t i = 0; i < m->entry_count; i++) {
        struct limine_memmap_entry *e = m->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;
        if (e->base < kbase + ksize && kbase < e->base + e->length)
            continue; /* don't shoot the kernel image */
        if (!best || e->length > best->length)
            best = e;
    }
    if (!best)
        return;

    size_t want = best->length < (64ULL << 20) ? (size_t)best->length : (64U << 20);
    heap_init((uintptr_t)best->base, want);
}

void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        for (;;)
            __asm__ volatile("cli; hlt");
    }

    console_init();
    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count >= 1) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        console_splash(fb);
        console_fb_init(fb); /* enable the on-screen text terminal */
    }

    gdt_init();
    idt_init();
    pic_init();
    timer_init(100);      /* 100 Hz */
    keyboard_init();
    init_heap();
    __asm__ volatile("sti"); /* Limine enters with IF=0: enable IRQs */

    kprintf("\n%s %s (%s) — FOSS Unix-like kernel\n",
            KELPOS_NAME, KELPOS_VERSION, KELPOS_ARCH);
    kprintf("Developed with AI assistance — see AI-POLICY.md\n");
    if (hhdm_request.response)
        kprintf("HHDM offset: 0x%llx\n", (unsigned long long)hhdm_request.response->offset);
    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count >= 1) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        kprintf("Framebuffer: %llux%llu @%llubpp — terminal enabled\n",
                (unsigned long long)fb->width,
                (unsigned long long)fb->height,
                (unsigned long long)fb->bpp);
    }
    kprintf("Shell ready — type 'help' for all commands.\n");
    kprintf("KELPOS_SMOKE_OK\n");

    shell_run();
}
