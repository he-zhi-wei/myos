#include "test.h"
#include "timer.h"

void test_pit_run(void) {
    serial_puts("--- Programmable Interval Timer ---\n");

    serial_puts("[PIT] ticks > 0...");
    uint32_t t1 = timer_ticks();
    test_result(t1 > 0);

    serial_puts("[PIT] ticks advancing...");
    for (volatile int d = 0; d < 500000; d++) {}
    uint32_t t2 = timer_ticks();
    test_result(t2 > t1);

    serial_puts("[PIT] rate ~100Hz...");
    test_result(t2 < 2000);
}
