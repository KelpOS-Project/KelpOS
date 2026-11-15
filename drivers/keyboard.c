/* drivers/keyboard.c — PS/2 keyboard (scancode set 1), IRQ1 -> vector 33 */
#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "keyboard.h"

static const char kmap_lower[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x27] = ';', [0x28] = '\'', [0x29] = '`', [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',', [0x34] = '.', [0x35] = '/', [0x39] = ' ',
};

static const char kmap_upper[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~', [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V',
    [0x30] = 'B', [0x31] = 'N', [0x32] = 'M',
    [0x33] = '<', [0x34] = '>', [0x35] = '?', [0x39] = ' ',
};

#define QSIZE 32
static volatile char queue[QSIZE];
static volatile int head, tail;
static bool shift;

void keyboard_init(void)
{
    /* drain stale scancodes from the controller */
    while (inb(0x64) & 0x01)
        (void)inb(0x60);
    head = tail = 0;
}

static void push(char c)
{
    int nh = (head + 1) % QSIZE;
    if (nh != tail) {
        queue[head] = c;
        head = nh;
    }
}

int kbd_getchar(void)
{
    for (;;) {
        if (tail != head) {
            int c = queue[tail];
            tail = (tail + 1) % QSIZE;
            return c;
        }
        __asm__ volatile("hlt");
    }
}

void keyboard_isr(void)
{
    uint8_t sc = inb(0x60);

    if (sc & 0x80) { /* key release */
        uint8_t k = sc & 0x7F;
        if (k == 0x2A || k == 0x36)
            shift = false;
        return;
    }

    if (sc == 0x2A || sc == 0x36) { /* shift pressed */
        shift = true;
        return;
    }

    char c = shift ? kmap_upper[sc] : kmap_lower[sc];
    if (c)
        push(c);
}
