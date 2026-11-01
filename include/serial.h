// 文件：serial.h
// 作用：串口输出接口，供内核在调试或运行时向串口打印字符与字符串。
// 要点：底层为 COM2（端口 0x2F8），serial_putc 轮询发送寄存器就绪位后再写入，serial_puts 逐字符输出；仅提供输出，无输入接口。
#pragma once

void serial_putc(char c);
void serial_puts(const char *s);
