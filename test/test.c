// 文件：test/test.c
// 作用：测试框架实现，统计通过/失败并输出结果行与结束标记。
// 要点：结果走 COM2 串口，VGA 同步显示摘要；仅 -DTEST 内核在 shell 前运行。

#include "test.h"
#include "vga.h"

int test_passed = 0;
int test_failed = 0;

void put_dec(unsigned int n) {
    if (n >= 10) put_dec(n / 10);
    serial_putc('0' + (n % 10));
}

static void vga_put_dec(unsigned int n) {
    if (n >= 10) vga_put_dec(n / 10);
    vga_putc('0' + (n % 10));
}

void test_check_line(const char *name, int ok, int line) {
    if (ok) {
        test_passed++;
        serial_puts("  [PASS] ");
    } else {
        test_failed++;
        serial_puts("  [FAIL] ");
    }
    serial_puts(name);
    if (!ok) {
        serial_puts(" (line ");
        put_dec((unsigned int)line);
        serial_putc(')');
    }
    serial_putc('\n');
}

void test_section(const char *title) {
    serial_puts("\n--- ");
    serial_puts(title);
    serial_puts(" ---\n");
}

int test_summary(void) {
    unsigned int total = (unsigned int)(test_passed + test_failed);

    serial_puts("\nRESULT ");
    put_dec((unsigned int)test_passed);
    serial_putc('/');
    put_dec(total);
    serial_puts(" passed\n");
    serial_puts(test_failed ? "TESTS_FAILED\n" : "TESTS_OK\n");

    vga_puts("\nTests: ");
    vga_put_dec((unsigned int)test_passed);
    vga_putc('/');
    vga_put_dec(total);
    vga_puts(" passed\n");

    return test_failed;
}

// 测试总入口：清零计数后依次运行各测试组，最后输出汇总。
void run_all_tests(void) {
    test_passed = 0;
    test_failed = 0;

    serial_puts("\n====== Unit Tests ======\n");
    vga_puts("\nTests running...\n");

    test_isr_run();
    test_pmm_run();
    test_pit_run();
    test_cmd_run();

    test_summary();
}
