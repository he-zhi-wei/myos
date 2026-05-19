#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "serial.h"

#define VGA_BUF ((volatile char *)0xB8000)

void kernel_main(unsigned int magic, unsigned int addr) {
    const char *msg = "Hello, OS World!";

    // Clear VGA screen
    for (int i = 0; i < 80 * 25; i++) {
        VGA_BUF[i * 2] = ' ';
        VGA_BUF[i * 2 + 1] = 0x07;
    }

    for (int i = 0; msg[i] != '\0'; i++) {
        VGA_BUF[i * 2] = msg[i];
        VGA_BUF[i * 2 + 1] = 0x07;
    }

    serial_puts(msg);
    serial_puts("\n");

    gdt_init();
    idt_init();
    irq_init();

    __asm__ volatile ("sti");

    serial_puts("Interrupts enabled.\n");

    for (;;) {}
}
