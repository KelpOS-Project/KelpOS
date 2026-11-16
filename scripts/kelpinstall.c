/* kelpinstall.c — the KelpOS installer (host side, plain C).
 *
 * Walks the user through:
 *   1. picking the target device (Device of Choice),
 *   2. picking the packages to build into the kernel,
 *   3. writing the disk: partition table, FAT32 boot partition,
 *      bootloader (Limine) install, ISO contents copied to disk.
 *
 * IT REALLY INSTALLS BY DEFAULT. The only no-write mode is the
 * explicit --dry-run flag, which just prints the plan. A real install
 * wipes the target device and requires root.
 *
 * Usage:
 *   kelpinstall                    # TUI, real install
 *   kelpinstall /dev/sdX           # TUI, device preselected
 *   kelpinstall --dry-run          # print the plan, write nothing
 *   kelpinstall --non-interactive /dev/sdX --packages fs,users,...
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define MNT   "/mnt/kelpos-install"
#define MAXDEV 32
#define MAXPKG 32

/* keep in sync with scripts/pkgconf.py */
static const char *const PKG_ALL[] = {
    "fs", "users", "fastfetch", "diskfs", "about",
    "mem", "crc", "cowsay", "fortune", "crash",
};
#define NPKG (int)(sizeof(PKG_ALL) / sizeof(PKG_ALL[0]))

/* ── terminal helpers (raw mode + ANSI escapes) ─────────────────────── */

static struct termios g_old_tio;
static int            g_raw = 0;

static void term_restore(void)
{
    if (g_raw) {
        tcsetattr(0, TCSAFLUSH, &g_old_tio);
        g_raw = 0;
    }
    printf("\x1b[?25h\x1b[0m");
    fflush(stdout);
}

static void term_raw(void)
{
    tcgetattr(0, &g_old_tio);
    g_raw = 1;
    atexit(term_restore);
    struct termios t = g_old_tio;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_iflag &= ~(IXON);
    tcsetattr(0, TCSAFLUSH, &t);
}

static void term_clear(void)
{
    printf("\x1b[2J\x1b[H");
}

/* one key press: 1=up, 2=down, 3=enter, 4=space, 5=esc, 0=other */
static int read_key(void)
{
    char c;
    ssize_t n = read(0, &c, 1);
    if (n <= 0)
        return 5; /* EOF/error: treat as esc so menus cannot spin */
    if (c == '\n' || c == '\r')
        return 3;
    if (c == ' ')
        return 4;
    if (c == 0x1b) {
        char seq[2];
        if (read(0, &seq[0], 1) != 1)
            return 5;
        if (read(0, &seq[1], 1) != 1)
            return 5;
        if (seq[0] == '[') {
            if (seq[1] == 'A')
                return 1;
            if (seq[1] == 'B')
                return 2;
        }
        return 0;
    }
    if (c == 'j')
        return 2;
    if (c == 'k')
        return 1;
    if (c == 'q')
        return 5;
    return 0;
}

static void dim(void)      { printf("\x1b[2m"); }
static void bold(void)     { printf("\x1b[1m"); }
static void green(void)    { printf("\x1b[32;1m"); }
static void red(void)      { printf("\x1b[31;1m"); }
static void attr_reset(void) { printf("\x1b[0m"); }

/* menu: returns index of the chosen item, -1 on ESC. Checkbox items
 * toggle with space and stay in the menu until enter. */
static int menu(const char *title, const char *const *items, int n,
                int *checked, int preselect)
{
    int cur = preselect >= 0 && preselect < n ? preselect : 0;
    if (cur >= n)
        cur = 0;

    for (;;) {
        term_clear();
        printf("\x1b[H");
        bold(); printf(" KelpOS Installer — %s\n\n", title); attr_reset();
        for (int i = 0; i < n; i++) {
            printf("  ");
            if (i == cur)
                printf("\x1b[7m");
            if (checked)
                printf("[%c] ", checked[i] ? 'x' : ' ');
            else
                printf("    ");
            printf("%s", items[i]);
            if (i == cur)
                printf("\x1b[27m");
            printf("\n");
        }
        printf("\n");
        dim();
        printf("  up/down: move   %senter: select%s   %sspace: toggle%s"
               "   esc: quit\n",
               checked ? "" : "", "", checked ? "" : "", "");
        attr_reset();
        fflush(stdout);

        int k = read_key();
        if (k == 1 && cur > 0)
            cur--;
        else if (k == 2 && cur < n - 1)
            cur++;
        else if (k == 4 && checked)
            checked[cur] = !checked[cur];
        else if (k == 3)
            return cur;
        else if (k == 5)
            return -1;
    }
}

/* ── device discovery (/sys/block) ──────────────────────────────────── */

static int dev_is_disk(const char *name)
{
    static const char *const skip[] = {
        "loop", "zram", "ram", "sr", "fd", "dm-", "md", "nbd", NULL
    };
    for (int i = 0; skip[i]; i++)
        if (strncmp(name, skip[i], strlen(skip[i])) == 0)
            return 0;
    return 1;
}

static unsigned long long dev_size_bytes(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/block/%s/size", name);
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    unsigned long long sectors = 0;
    if (fscanf(f, "%llu", &sectors) != 1)
        sectors = 0;
    fclose(f);
    return sectors * 512;
}

static int list_devices(char devs[MAXDEV][64], unsigned long long sz[MAXDEV])
{
    DIR *d = opendir("/sys/block");
    if (!d)
        return 0;
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) && n < MAXDEV) {
        if (!dev_is_disk(e->d_name))
            continue;
        char namebuf[64];
        strncpy(namebuf, e->d_name, sizeof(namebuf) - 1);
        namebuf[sizeof(namebuf) - 1] = 0;
        snprintf(devs[n], 64, "/dev/%.53s", namebuf);
        sz[n] = dev_size_bytes(e->d_name);
        n++;
    }
    closedir(d);
    /* sort: simple insertion by name */
    for (int i = 1; i < n; i++) {
        char tmp[64];
        unsigned long long tsz = sz[i];
        strcpy(tmp, devs[i]);
        int j = i - 1;
        while (j >= 0 && strcmp(devs[j], tmp) > 0) {
            strcpy(devs[j + 1], devs[j]);
            sz[j + 1] = sz[j];
            j--;
        }
        strcpy(devs[j + 1], tmp);
        sz[j + 1] = tsz;
    }
    return n;
}

static void human_size(unsigned long long b, char *out, size_t cap)
{
    if (b >= 10ULL * 1024 * 1024 * 1024)
        snprintf(out, cap, "%.1f GiB", (double)b / (1024.0 * 1024 * 1024));
    else if (b >= 10ULL * 1024 * 1024)
        snprintf(out, cap, "%.0f MiB", (double)b / (1024.0 * 1024));
    else
        snprintf(out, cap, "%llu KiB", b / 1024);
}

/* ── repo-relative helpers ──────────────────────────────────────────── */

static const char *find_repo(void)
{
    static char buf[512];
    /* binary lives in <repo>/build/kelpinstall or <repo>/scripts/... */
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) {
        strcpy(buf, ".");
        return buf;
    }
    buf[n] = 0;
    char *slash = strrchr(buf, '/');
    if (slash)
        *slash = 0;
    slash = strrchr(buf, '/');
    if (slash && (!strcmp(slash + 1, "build") || !strcmp(slash + 1, "scripts")))
        *slash = 0;
    return buf;
}

static int run(const char *cmd, const char *what)
{
    printf("  $ %s\n", cmd);
    fflush(stdout);
    int rc = system(cmd);
    if (rc != 0) {
        red();
        if (WIFEXITED(rc) && WEXITSTATUS(rc) == 1 && geteuid() != 0)
            printf("kelpinstall: '%s' failed (rc=1) — not root?\n", what);
        else
            printf("kelpinstall: '%s' failed (rc=%d)\n", what, rc);
        attr_reset();
    }
    return rc;
}

static int tool_exists(const char *name)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

/* ── the install plan ───────────────────────────────────────────────── */

static int install_real(const char *dev);

/* print the plan; with dry==0 perform the real install */
static int install(const char *dev, const char *pkgs, int dry)
{
    const char *repo = find_repo();
    char limine[512], iso[512];

    snprintf(limine, sizeof(limine), "%s/build/limine/limine", repo);
    snprintf(iso, sizeof(iso), "%s/build/iso", repo);

    printf("\n");
    bold(); printf("KelpOS install plan for %s\n", dev); attr_reset();
    printf("  packages: %s\n", pkgs ? pkgs : "(image already built)");
    printf("  1. partition %s (MBR, FAT32 at LBA 2048, bootable)\n", dev);
    printf("  2. mkfs.vfat -F 32 %s1\n", dev);
    printf("  3. mount %s1 -> %s\n", dev, MNT);
    printf("  4. copy %s/* (kernel, limine.conf, wallpaper, disk.img)\n", iso);
    printf("  5. unmount\n");
    printf("  6. %s bios-install %s   (MBR bootloader)\n", limine, dev);

    if (dry) {
        printf("\n  dry run: nothing was written.\n");
        return 0;
    }

    if (geteuid() != 0) {
        red();
        printf("\nkelpinstall: installing needs root — run with sudo.\n");
        attr_reset();
        return 1;
    }
    if (!tool_exists("sfdisk") ||
        !(tool_exists("mkfs.vfat") || tool_exists("mkfs.fat")) ||
        access(limine, X_OK) != 0) {
        red();
        printf("\nkelpinstall: missing tools (need sfdisk, mkfs.vfat, "
               "build/limine/limine — run 'make limine' first).\n");
        attr_reset();
        return 1;
    }

    return install_real(dev);
}

/* real partitioning/format/copy step, split out for readable errors */
static int install_real(const char *dev)
{
    char cmd[2048];
    int rc;

    /* 1. partition table: single FAT32 partition, LBA 2048, bootable */
    snprintf(cmd, sizeof(cmd),
             "sfdisk --wipe=always %s <<'EOF'\n"
             "label: dos\n"
             "start=2048, type=0c, bootable\n"
             "EOF", dev);
    if (run(cmd, "partitioning"))
        return 1;

    /* 2. filesystem */
    snprintf(cmd, sizeof(cmd), "mkfs.vfat -F 32 %s1", dev);
    if (run(cmd, "mkfs.vfat"))
        return 1;

    /* 3-5. mount, copy payload, unmount */
    snprintf(cmd, sizeof(cmd),
             "mkdir -p %s && mount %s1 %s", MNT, dev, MNT);
    if (run(cmd, "mount"))
        return 1;
    snprintf(cmd, sizeof(cmd),
             "cp -r %s/build/iso/. %s/ && sync", find_repo(), MNT);
    rc = run(cmd, "copying payload");
    snprintf(cmd, sizeof(cmd), "umount %s", MNT);
    if (rc || run(cmd, "unmounting"))
        return 1;

    /* 6. bootloader MBR */
    snprintf(cmd, sizeof(cmd), "%s/build/limine/limine bios-install %s",
             find_repo(), dev);
    if (run(cmd, "Limine BIOS install"))
        return 1;

    green();
    printf("\nDone. KelpOS is installed on %s — reboot to boot it.\n", dev);
    attr_reset();
    return 0;
}

/* ── main ───────────────────────────────────────────────────────────── */

static int parse_packages(const char *arg, int *checked)
{
    for (int i = 0; i < NPKG; i++)
        checked[i] = 0;
    if (!strcmp(arg, "ALL") || !strcmp(arg, "all")) {
        for (int i = 0; i < NPKG; i++)
            checked[i] = 1;
        return 0;
    }
    char buf[512];
    strncpy(buf, arg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    for (char *p = strtok(buf, ","); p; p = strtok(NULL, ",")) {
        int hit = 0;
        for (int i = 0; i < NPKG; i++)
            if (!strcmp(p, PKG_ALL[i])) {
                checked[i] = 1;
                hit = 1;
            }
        if (!hit) {
            fprintf(stderr, "kelpinstall: unknown package '%s'\n", p);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *device = NULL;
    const char *packages = NULL;
    int dry = 0, non_interactive = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--dry-run") || !strcmp(argv[i], "-n"))
            dry = 1;
        else if (!strcmp(argv[i], "--non-interactive"))
            non_interactive = 1;
        else if (!strcmp(argv[i], "--packages") && i + 1 < argc)
            packages = argv[++i];
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: kelpinstall [--dry-run] [--non-interactive "
                   "DEVICE --packages LIST|ALL]\n");
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "kelpinstall: unknown option %s\n", argv[i]);
            return 2;
        } else
            device = argv[i];
    }

    if (non_interactive && (!device || !packages)) {
        fprintf(stderr, "kelpinstall: --non-interactive needs a device "
                        "and --packages\n");
        return 2;
    }

    /* the TUI needs a real terminal — only required when we actually
     * have something to ask (no device and/or no package list) */
    if (!non_interactive && !isatty(0) && !dry && (!device || !packages)) {
        fprintf(stderr, "kelpinstall: stdin is not a terminal — "
                        "use --non-interactive DEVICE --packages LIST|ALL\n");
        return 2;
    }

    int checked[NPKG];
    char pkgbuf[512] = "";
    char (*devs)[64] = calloc(MAXDEV, 64);
    unsigned long long *dsz = calloc(MAXDEV, sizeof(*dsz));
    int ndev = list_devices(devs, dsz);

    term_raw();

    if (!non_interactive) {
        /* 1. device of choice */
        if (device) {
            /* preselected device: still show the summary */
        } else if (ndev == 0) {
            term_restore();
            red();
            printf("kelpinstall: no block devices found in /sys/block\n");
            attr_reset();
            return 1;
        } else if (isatty(0)) {
            char *items[MAXDEV];
            char labels[MAXDEV][96];
            for (int i = 0; i < ndev; i++) {
                char hs[24];
                human_size(dsz[i], hs, sizeof(hs));
                snprintf(labels[i], sizeof(labels[i]), "%s  (%s)",
                         devs[i], hs);
                items[i] = labels[i];
            }
            term_restore();
            int r = menu("Device of Choice", (const char *const *)items,
                         ndev, NULL, -1);
            term_raw();
            if (r < 0) {
                term_restore();
                printf("kelpinstall: aborted.\n");
                return 1;
            }
            device = devs[r];
        }

        /* 2. packages: menu on a tty, otherwise defaults or --packages */
        for (int i = 0; i < NPKG; i++)
            checked[i] = 1;
        if (packages)
            parse_packages(packages, checked);
        if (isatty(0)) {
            term_restore();
            int r = menu("Packages", PKG_ALL, NPKG, checked, -1);
            term_raw();
            if (r < 0) {
                term_restore();
                printf("kelpinstall: aborted.\n");
                return 1;
            }
        }
        size_t o = 0;
        for (int i = 0; i < NPKG; i++)
            if (checked[i])
                o += (size_t)snprintf(pkgbuf + o, sizeof(pkgbuf) - o,
                                      "%s%s", o ? "," : "", PKG_ALL[i]);
        packages = pkgbuf;
    } else {
        if (parse_packages(packages, checked))
            return 2;
        size_t o = 0;
        for (int i = 0; i < NPKG; i++)
            if (checked[i])
                o += (size_t)snprintf(pkgbuf + o, sizeof(pkgbuf) - o,
                                      "%s%s", o ? "," : "", PKG_ALL[i]);
        packages = pkgbuf;
    }

    term_restore();

    if (dry) {
        printf("kelpinstall: DRY RUN — printing the plan only.\n");
        return install(device, packages, 1);
    }

    /* confirmation: the disk WILL be erased */
    printf("\n");
    red();
    printf("WARNING: %s will be ERASED and KelpOS installed.\n", device);
    attr_reset();
    printf("Packages: %s\n", packages);
    printf("Type the device name to confirm (%s): ", device);
    fflush(stdout);
    char conf[128] = "";
    if (!fgets(conf, sizeof(conf), stdin) ||
        conf[strspn(conf, " \t")] == 0) {
        printf("kelpinstall: aborted.\n");
        return 1;
    }
    /* strip newline */
    conf[strcspn(conf, "\r\n")] = 0;
    if (strcmp(conf, device) != 0) {
        printf("kelpinstall: aborted (confirmation did not match).\n");
        return 1;
    }

    return install_real(device);
}
