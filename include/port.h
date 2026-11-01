// 文件：port.h
// 作用：x86 端口 I/O 的内联汇编封装（inb/outb/outw），供键盘、PIC、PIT、串口等驱动读写设备寄存器。
// 要点：均为 static inline + volatile asm，端口操作数用 "Nd" 约束；outb 写 8 位、outw 写 16 位，均无内存屏障，寄存器依赖顺序时由调用方自行保证。
#pragma once
#include "types.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
