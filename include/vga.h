// 文件：vga.h
// 作用：VGA 文本模式（80x25，显存 0xB8000）的输出与光标控制接口，供 shell 和内核打印信息。
// 要点：坐标以字符格为单位，行列均从 0 起算；vga_set_color 设前景/背景色并影响后续输出，vga_clear_line(y, from) 从第 from 列清到该行结尾；输出到第 25 行会自动滚屏。
#pragma once
#include "types.h"

void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char *s);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_set_cursor(int x, int y);
int  vga_get_cursor_x(void);
int  vga_get_cursor_y(void);
void vga_clear_line(int y, int from);
