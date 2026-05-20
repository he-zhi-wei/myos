#pragma once
#include "types.h"

void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char *s);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_set_cursor(int x, int y);
int  vga_get_cursor_x(void);
int  vga_get_cursor_y(void);
