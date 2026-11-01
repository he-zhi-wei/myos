// 文件：serial.c
// 作用：COM2（0x2F8）串口输出，用于在 VGA 之外记录调试日志。
// 要点：只实现输出，serial_putc 轮询线路状态寄存器 +5 的 THR 空位后写入数据；
//       被 kernel/shell/isr 等广泛调用，本文件未做波特率等初始化，依赖固件或模拟器的默认配置。

#include "port.h"

#define SER_PORT 0x2F8   // COM2

void serial_putc(char c) {
    while ((inb(SER_PORT + 5) & 0x20) == 0) {}
    outb(SER_PORT, c);
}

void serial_puts(const char *s) {
    for (; *s; s++) serial_putc(*s);
}
