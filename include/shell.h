// 文件：shell.h
// 作用：内核命令行 shell 的初始化、逐字符输入与提示符接口，供键盘中断和主循环驱动。
// 要点：shell_input 接收单个按键而非整行；shell_input_suspended 非 0 表示 Ring 3 程序独占键盘，shell 不得消费按键，退出用户程序后再恢复。
#pragma once

void shell_init(void);
void shell_input(int c);
void shell_prompt(void);

// Set while a Ring 3 program owns the keyboard; the shell must not consume keys.
extern int shell_input_suspended;
