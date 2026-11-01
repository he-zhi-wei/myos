// 文件：irq.h
// 作用：硬件中断 IRQ0~IRQ15 的注册与分发接口，供 interrupt.asm 的 irq_common 转到各设备驱动回调。
// 要点：回调类型 irq_fn 接收 registers_t*；irq_register_handler 按 IRQ 号挂接，irq_unmask 通过 PIC 解除对应屏蔽位。
#pragma once
#include "isr.h"

typedef void (*irq_fn)(registers_t *);

void irq_init(void);
void irq_handler(registers_t *regs);
void irq_register_handler(int irq, irq_fn handler);
void irq_unmask(int irq);
