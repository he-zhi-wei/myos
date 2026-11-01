// 文件：test/pit.c
// 作用：验证 100Hz PIT 定时器的节拍计数与 uptime 换算。
// 要点：依赖 timer_ticks/timer_freq，用 sti; hlt 等待节拍推进。

#include "test.h"
#include "timer.h"

void test_pit_run(void) {
    test_section("Programmable Interval Timer");

    TEST("PIT is programmed at 100Hz", timer_freq() == 100);

    uint32_t t1 = timer_ticks();
    while (timer_ticks() == t1) { __asm__ volatile ("sti; hlt"); }
    TEST("tick counter advances while idling", timer_ticks() > t1);

    t1 = timer_ticks();
    while (timer_ticks() < t1 + 10) { __asm__ volatile ("sti; hlt"); }
    TEST("waiting for 10 ticks counts exactly 10", timer_ticks() == t1 + 10);

    TEST("uptime_sec equals ticks/freq", timer_uptime_sec() == timer_ticks() / timer_freq());
}
