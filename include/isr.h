// 文件：isr.h
// 作用：定义统一中断栈帧 registers_t 与异常分发入口 isr_handler，供 interrupt.asm 传参及 C 侧处理异常。
// 要点：字段顺序必须与 interrupt.asm 的压栈顺序严格一致（DS → pusha 的 edi..eax → int_no/err_code → CPU 压入的 eip/cs/eflags/useresp/ss），改动任一顺序都会让字段解析错位；结构体须 packed。
#pragma once
#include "types.h"

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

void isr_handler(registers_t *regs);
