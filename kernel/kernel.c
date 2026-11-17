/* kernel/kernel.c — entry after _start (boot.S): boots via the Limine protocol */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <limine.h>
#include "version.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "shell.h"
#include "heap.h"
#include "ramfs.h"
#include "users.h"
#include "desktop.h"
#include "block.h"
#include "vfs.h"
#include "ramdisk.h"

void diskfs_init(void); /* kernel/bin/mounts.c: registers fs backends */
void kpm_restore(void); /* kernel/bin/kpm.c: package DB from last boot */

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

/* e.g. the "KelpOS (Silent Boot)" entry ships a silent.flag module */
static bool boot_flag(const char *needle)
{
    struct limine_module_response *r = module_request.response;
    if (!r)
        return false;
    for (uint64_t i = 0; i < r->module_count; i++)
        if (r->modules[i] && r->modules[i]->path &&
            strstr(r->modules[i]->path, needle))
            return true;
    return false;
}

/* find a boot module by path substring; returns its (HHDM) address
 * and size, or NULL. Used for e.g. the desktop wallpaper BMP. */
const void *boot_module(const char *needle, size_t *size)
{
    struct limine_module_response *r = module_request.response;
    if (!r)
        return NULL;
    for (uint64_t i = 0; i < r->module_count; i++) {
        struct limine_file *m = r->modules[i];
        if (!m || !m->path || !strstr(m->path, needle))
            continue;
        if (size)
            *size = (size_t)m->size;
        return (const void *)(uintptr_t)m->address;
    }
    return NULL;
}

/* block layer + VFS: mount the disk.img module (a FAT image shipped in
 * the ISO) as the first real filesystem at /media/disk0 */
static void init_diskfs(void)
{
    block_init();
    vfs_init();
    diskfs_init(); /* registers fat / ext4 / btrfs backend ops */

    struct limine_module_response *r = module_request.response;
    if (!r)
        return;

    for (uint64_t i = 0; i < r->module_count; i++) {
        struct limine_file *m = r->modules[i];
        if (!m || !m->path || !strstr(m->path, "disk.img"))
            continue;
        /* limine_file.address is already an HHDM virtual address */
        if (ramdisk_register("ram0", (uintptr_t)m->address, m->size) == 0) {
            struct blockdev *bd = block_find("ram0");
            if (bd && vfs_mount(bd, "/media/disk0") == 0)
                kprintf("diskfs: mounted %s (%s) at /media/disk0\n",
                        bd->name, vfs_mount_get(0)->ops->name);
        }
        break;
    }
}

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
    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count >= 1) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        mouse_init((int)fb->width, (int)fb->height);
    }

    /* ramfs + users (users_sync mirrors the DB into /etc) */
    ramfs_init();
    ramfs_mkdir("/etc");
    ramfs_mkdir("/etc/kpm");
    ramfs_mkdir("/home");
    ramfs_mkdir("/home/root");
    ramfs_mkdir("/tmp");
    ramfs_mkfile("/etc/motd");
    ramfs_mkfile("/etc/kpm/installed");
    struct ramfs_node *motd = ramfs_resolve_at(ramfs_root(), "/etc/motd");
    if (motd)
        ramfs_write(motd, "Welcome to KelpOS - the FOSS Unix-like kernel.\n", 47);
    users_init();
    init_diskfs();
    kpm_restore(); /* re-enable the packages installed last boot */

    __asm__ volatile("sti"); /* Limine enters with IF=0: enable IRQs */

    bool silent = boot_flag("silent");
    if (!silent) {
        kprintf("\n%s %s (%s) — FOSS Unix-like kernel\n",
                KELPOS_NAME, KELPOS_VERSION, KELPOS_ARCH);
        kprintf("Developed with AI assistance — see AI-POLICY.md\n");
        if (hhdm_request.response)
            kprintf("HHDM offset: 0x%llx\n",
                    (unsigned long long)hhdm_request.response->offset);
        if (framebuffer_request.response && framebuffer_request.response->framebuffer_count >= 1) {
            struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
            kprintf("Framebuffer: %llux%llu @%llubpp — terminal enabled\n",
                    (unsigned long long)fb->width,
                    (unsigned long long)fb->height,
                    (unsigned long long)fb->bpp);
        }
        kprintf("Shell ready — type 'help' for all commands.\n");
        kprintf("KELPOS_SMOKE_OK\n");
        shell_run(); /* noreturn */
    }

    /* silent boot: no banner, straight into the desktop */
    console_clear();
    desktop_run(); /* noreturn */
}
