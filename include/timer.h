// 文件：timer.h
// 作用：PIT 定时器初始化与节拍查询接口，为任务调度、sleep 与 uptime 提供时间基准。
// 要点：timer_init 设定每秒中断频率 freq_hz 并开启 IRQ0；timer_ticks 返回累计节拍，timer_uptime_sec 按频率换算秒数，timer_handler 由 IRQ 分发转入。
#pragma once
#include "types.h"
#include "isr.h"

void timer_init(uint32_t freq_hz);
void timer_handler(registers_t *regs);
uint32_t timer_ticks(void);
uint32_t timer_freq(void);
uint32_t timer_uptime_sec(void);
