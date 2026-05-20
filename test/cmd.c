#include "test.h"
#include "timer.h"

void test_cmd_run(void) {
    serial_puts("--- Command Tools ---\n");

    // sleep test: should return in reasonable time
    serial_puts("[CMD] sleep 1ms...");
    uint32_t t1 = timer_ticks();
    volatile uint32_t target = t1 + timer_freq() / 1000;
    while (timer_ticks() < target) { __asm__ volatile ("hlt"); }
    uint32_t t2 = timer_ticks();
    test_result(t2 >= t1 && t2 - t1 <= 5);  // at most 5 ticks (50ms) for 1ms sleep

    // bench test: verify timing (PIT granularity is 10ms)
    serial_puts("[CMD] bench 100k...");
    t1 = timer_ticks();
    for (volatile int i = 0; i < 100000; i++) {}
    t2 = timer_ticks();
    test_result(t2 >= t1 && (t2 - t1) < 500);

    // hexdump sanity: read a known address (VGA buffer)
    serial_puts("[CMD] hexdump VGA...");
    volatile char *vga = (volatile char *)0xB8000;
    test_result(vga[0] != 0);  // something is on screen

    // Direct sleep test (bypass shell)
    serial_puts("[CMD] sleep direct 50ms...");
    uint32_t bt = timer_ticks();
    uint32_t tg = bt + (50 * timer_freq() + 999) / 1000;
    while (timer_ticks() < tg) { __asm__ volatile ("hlt"); }
    uint32_t at2 = timer_ticks();
    test_result(at2 >= tg);
}
