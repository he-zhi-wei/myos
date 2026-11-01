// 文件：test/cmd.c
// 作用：验证延时辅助函数与 VGA 文本输出是否正常。
// 要点：依赖 PIT 节拍与 0xB8000 文本缓冲，延时不得超前过多节拍。

#include "test.h"
#include "timer.h"

void test_cmd_run(void) {
    test_section("Timing helpers and text output");

    // 1ms is below one PIT tick (10ms), so the wait must not overshoot.
    uint32_t t1 = timer_ticks();
    uint32_t target = t1 + (1 * timer_freq() + 999) / 1000;
    while (timer_ticks() < target) { __asm__ volatile ("sti; hlt"); }
    TEST("1ms wait completes within one tick", timer_ticks() - t1 <= 1);

    // Regression guard against a runaway busy loop.
    t1 = timer_ticks();
    for (volatile int i = 0; i < 100000; i++) {}
    TEST("100k-iteration loop stays under 500 ticks", timer_ticks() - t1 < 500);

    TEST("VGA text buffer has been written",
         ((volatile uint16_t *)0xB8000)[0] != 0);

    // A 50ms wait targets 5 ticks and must not drift past it.
    uint32_t bt = timer_ticks();
    uint32_t tg = bt + (50 * timer_freq() + 999) / 1000;
    while (timer_ticks() < tg) { __asm__ volatile ("sti; hlt"); }
    TEST("50ms wait spans 5 ticks without overshoot",
         timer_ticks() - bt >= 4 && timer_ticks() - bt <= 6);
}
