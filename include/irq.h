#pragma once
#include "isr.h"

typedef void (*irq_fn)(registers_t *);

void irq_init(void);
void irq_handler(registers_t *regs);
void irq_register_handler(int irq, irq_fn handler);
void irq_unmask(int irq);
