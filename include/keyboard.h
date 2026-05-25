#pragma once
#include "isr.h"

void keyboard_init(void);
void keyboard_handler(registers_t *regs);

// Keyboard buffer for user-mode SYS_READ / SYS_GETC
void kb_putc(char c);
int  kb_getc(void);   // blocking read
int  kb_poll(void);   // non-blocking: returns char, or -1 if buffer empty
