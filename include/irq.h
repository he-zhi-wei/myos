#pragma once
#include "isr.h"

void irq_init(void);
void irq_handler(registers_t *regs);
