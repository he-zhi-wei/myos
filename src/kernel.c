#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "serial.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "test.h"

void kernel_main(unsigned int magic, unsigned int addr) {
    vga_clear();
    vga_puts("Hello, OS World!\n");

    serial_puts("Hello, OS World!\n");

    gdt_init();
    idt_init();
    irq_init();

    __asm__ volatile ("sti");

    serial_puts("Interrupts enabled.\n");
    vga_puts("Interrupts enabled.\n");

    run_tests();

    // Enable keyboard → shell
    keyboard_init();
    irq_register_handler(1, keyboard_handler);
    irq_unmask(1);

    vga_putc('\n');
    shell_init();

    for (;;) {}
}
