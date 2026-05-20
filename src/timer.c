#include "timer.h"
#include "port.h"
#include "isr.h"

static uint32_t ticks = 0;
static uint32_t freq  = 0;

void timer_handler(registers_t *regs) {
    (void)regs;
    ticks++;
}

void timer_init(uint32_t freq_hz) {
    freq = freq_hz;
    uint32_t divisor = 1193180 / freq_hz;

    outb(0x43, 0x36);                  // channel 0, lobyte/hibyte, mode 2
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

uint32_t timer_ticks(void)   { return ticks; }
uint32_t timer_freq(void)    { return freq; }
uint32_t timer_uptime_sec(void) { return ticks / freq; }
