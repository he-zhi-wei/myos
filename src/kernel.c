#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "serial.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "test.h"
#include "pmm.h"
#include "paging.h"
#include "timer.h"

void memory_init(uint32_t mb_addr);

void kernel_main(unsigned int magic, unsigned int addr) {
    vga_clear();
    vga_puts("Hello, OS World!\n");
    serial_puts("Hello, OS World!\n");

    gdt_init();
    idt_init();
    irq_init();

    memory_init(addr);

    // PIT timer at 100Hz
    timer_init(100);
    irq_register_handler(0, timer_handler);
    irq_unmask(0);

    __asm__ volatile ("sti");

    serial_puts("Interrupts enabled.\n");
    vga_puts("Interrupts enabled.\n");

    run_tests();

    keyboard_init();
    irq_register_handler(1, keyboard_handler);
    irq_unmask(1);

    vga_putc('\n');
    shell_init();

    for (;;) {}
}
