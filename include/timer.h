#pragma once
#include "types.h"
#include "isr.h"

void timer_init(uint32_t freq_hz);
void timer_handler(registers_t *regs);
uint32_t timer_ticks(void);
uint32_t timer_freq(void);
uint32_t timer_uptime_sec(void);
