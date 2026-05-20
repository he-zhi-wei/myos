#include "test.h"

static void test_isr_noerr(int num) {
    serial_puts("[ISR_NOERR] int $");
    put_dec(num);
    serial_puts("...");

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

void test_isr_run(void) {
    serial_puts("--- ISR trap path (no error code, handler returns) ---\n");
    test_isr_noerr(0);
    test_isr_noerr(1);
    test_isr_noerr(3);
    test_isr_noerr(4);
    test_isr_noerr(6);
    test_isr_noerr(7);

    serial_puts("\n--- IRQ path (stub + irq_handler + EOI) ---\n");
    test_irq_path(0);
    test_irq_path(1);
    test_irq_path(2);
    test_irq_path(7);
    test_irq_path(15);
}
