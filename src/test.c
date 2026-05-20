#include "../test/test.h"

int test_passed = 0;
int test_failed = 0;

extern void test_isr_run(void);
extern void test_pmm_run(void);
extern void test_pit_run(void);
extern void test_cmd_run(void);

void run_tests(void) {
    test_passed = 0;
    test_failed = 0;

    serial_puts("\n====== Unit Tests ======\n\n");
    vga_puts("\nTests running...\n");

    test_isr_run();
    serial_puts("\n");
    test_pmm_run();
    serial_puts("\n");
    test_pit_run();
    serial_puts("\n");
    test_cmd_run();

    serial_puts("\n====== ");
    put_dec(test_passed);
    serial_puts("/");
    put_dec(test_passed + test_failed);
    serial_puts(" tests passed ======\n");

    vga_puts("Tests: ");
    if (test_passed >= 10) vga_putc('0' + test_passed / 10);
    vga_putc('0' + test_passed % 10);
    vga_puts("/");
    if ((test_passed + test_failed) >= 10) vga_putc('0' + (test_passed + test_failed) / 10);
    vga_putc('0' + (test_passed + test_failed) % 10);
    vga_puts(" passed\n");
}
