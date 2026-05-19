#include "test.h"
#include "serial.h"
#include "vga.h"

static int passed = 0;
static int failed = 0;

static void put_dec(unsigned int n) {
    if (n >= 10) put_dec(n / 10);
    serial_putc('0' + (n % 10));
}

static void test_result(int ok) {
    if (ok) { passed++; serial_puts("  -> PASS\n"); }
    else    { failed++; serial_puts("  -> FAIL\n"); }
}

// Test ISR_NOERR path: software interrupt to vectors 0–7, 9, 15–16, 18–29, 31
static void test_isr_noerr(int num) {
    serial_puts("[ISR_NOERR] int $");
    put_dec(num);
    serial_puts("...");

    // Save return point: int $n pushes next EIP; handler returns via iret
    switch (num) {
        case 0:  __asm__ volatile ("int $0");  break;
        case 1:  __asm__ volatile ("int $1");  break;
        case 2:  __asm__ volatile ("int $2");  break;
        case 3:  __asm__ volatile ("int $3");  break;
        case 4:  __asm__ volatile ("int $4");  break;
        case 5:  __asm__ volatile ("int $5");  break;
        case 6:  __asm__ volatile ("int $6");  break;
        case 7:  __asm__ volatile ("int $7");  break;
        case 9:  __asm__ volatile ("int $9");  break;
        case 15: __asm__ volatile ("int $15"); break;
        case 16: __asm__ volatile ("int $16"); break;
        case 18: __asm__ volatile ("int $18"); break;
        case 19: __asm__ volatile ("int $19"); break;
        default: return;
    }
    test_result(1);
}

// Test IRQ path: software interrupt to IRQ vectors (32–47)
static void test_irq_path(int irq) {
    serial_puts("[IRQ] int $");
    put_dec(32 + irq);
    serial_puts(" (IRQ");
    put_dec(irq);
    serial_puts(")...");

    switch (irq) {
        case 0:  __asm__ volatile ("int $32"); break;
        case 1:  __asm__ volatile ("int $33"); break;
        case 2:  __asm__ volatile ("int $34"); break;
        case 3:  __asm__ volatile ("int $35"); break;
        case 7:  __asm__ volatile ("int $39"); break;
        case 15: __asm__ volatile ("int $47"); break;
        default: return;
    }
    test_result(1);
}

void run_tests(void) {
    serial_puts("\n====== Interrupt Subsystem Tests ======\n\n");

    // Group 1: ISR_NOERR — verify handlers don't crash, return correctly
    serial_puts("--- ISR trap path (no error code, handler returns) ---\n");
    test_isr_noerr(0);   // division by zero
    test_isr_noerr(1);   // debug
    test_isr_noerr(3);   // breakpoint
    test_isr_noerr(4);   // overflow
    test_isr_noerr(6);   // invalid opcode
    test_isr_noerr(7);   // device not available

    // Group 2: IRQ — verify stub + PIC EOI + handler return
    serial_puts("\n--- IRQ path (stub + irq_handler + EOI) ---\n");
    test_irq_path(0);    // timer
    test_irq_path(1);    // keyboard
    test_irq_path(2);    // cascade
    test_irq_path(7);    // LPT1
    test_irq_path(15);   // ATA

    serial_puts("\n====== ");
    put_dec(passed);
    serial_puts("/");
    put_dec(passed + failed);
    serial_puts(" tests passed ======\n");

    vga_puts("\nTests: ");
    vga_putc('0' + passed / 10);
    vga_putc('0' + passed % 10);
    vga_puts("/");
    vga_putc('0' + (passed + failed) / 10);
    vga_putc('0' + (passed + failed) % 10);
    vga_puts(" passed\n");
}
