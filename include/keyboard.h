// 文件：keyboard.h
// 作用：PS/2 键盘初始化、IRQ1 中断处理与按键缓冲接口，供 shell 及用户态 SYS_READ / SYS_GETC 取字符。
// 要点：键盘中断经 kb_putc 写入缓冲；kb_getc 阻塞读，kb_poll 空缓冲返回 -1，kb_flush 清空全部缓冲输入。
#pragma once
#include "isr.h"

void keyboard_init(void);
void keyboard_handler(registers_t *regs);

// Keyboard buffer for user-mode SYS_READ / SYS_GETC
void kb_putc(char c);
int  kb_getc(void);   // blocking read
int  kb_poll(void);   // non-blocking: returns char, or -1 if buffer empty
void kb_flush(void);  // discard all buffered input
