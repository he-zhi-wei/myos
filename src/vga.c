#include "vga.h"

#define VGA_BUF ((volatile char *)0xB8000)

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t attr = 0x07;

void vga_set_color(uint8_t fg, uint8_t bg) {
    attr = (bg << 4) | (fg & 0x0F);
}

static void scroll(void) {
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 80; x++) {
            VGA_BUF[(y * 80 + x) * 2]     = VGA_BUF[((y + 1) * 80 + x) * 2];
            VGA_BUF[(y * 80 + x) * 2 + 1] = VGA_BUF[((y + 1) * 80 + x) * 2 + 1];
        }
    for (int x = 0; x < 80; x++) {
        VGA_BUF[(24 * 80 + x) * 2]     = ' ';
        VGA_BUF[(24 * 80 + x) * 2 + 1] = attr;
    }
    cursor_y = 24;
}

void vga_clear(void) {
    for (int i = 0; i < 80 * 25; i++) {
        VGA_BUF[i * 2]     = ' ';
        VGA_BUF[i * 2 + 1] = attr;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        else if (cursor_y > 0) { cursor_x = 79; cursor_y--; }
        VGA_BUF[(cursor_y * 80 + cursor_x) * 2]     = ' ';
        VGA_BUF[(cursor_y * 80 + cursor_x) * 2 + 1] = attr;
    } else {
        VGA_BUF[(cursor_y * 80 + cursor_x) * 2]     = c;
        VGA_BUF[(cursor_y * 80 + cursor_x) * 2 + 1] = attr;
        cursor_x++;
        if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }
    }
    if (cursor_y >= 25) scroll();
}

void vga_puts(const char *s) {
    for (; *s; s++) vga_putc(*s);
}
