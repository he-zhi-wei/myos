#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "serial.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"
#include "tss.h"
#include "fs.h"
#include "elf.h"

#ifdef TEST
#include "test.h"
#endif

void memory_init(uint32_t mb_addr);

// Embedded user ELF binary (embed.asm: incbin user.elf)
extern uint8_t user_bin_start[];
extern uint8_t user_bin_end[];

// Return context for SYS_EXIT — saved before launching first user task
uint32_t ret_eip, ret_cs, ret_eflags;

void kernel_main(unsigned int magic, unsigned int addr) {
    vga_clear();
    vga_puts("Hello, OS World!\n");
    serial_puts("Hello, OS World!\n");

    gdt_init();
    tss_init();
    fs_init();
    idt_init();
    irq_init();
    memory_init(addr);

    // Load the embedded user binary into RamFS so it can be exec'd later
    {
        int fd = fs_open("user_prog", 1);
        uint32_t sz = (uint32_t)user_bin_end - (uint32_t)user_bin_start;
        if (fd >= 0 && sz > 0) {
            fs_write(fd, (const char *)user_bin_start, sz);
            fs_close(fd);
        }
    }

    timer_init(100);
    irq_register_handler(0, timer_handler);
    irq_unmask(0);

    __asm__ volatile ("sti");

    serial_puts("Interrupts enabled.\n");
    vga_puts("Interrupts enabled.\n");

#ifdef TEST
    run_all_tests();
#endif

    // Launch the first user program from RamFS.
    // elf_exec never returns on success — SYS_EXIT brings us back here.
    ret_eflags = 0x202;
    ret_cs     = GDT_SEL_CODE0;
    __asm__ volatile ("movl $1f, %0" : "=r"(ret_eip));
    if (elf_exec("user_prog") < 0) {
        vga_puts("[exec user_prog failed]\n");
        serial_puts("[exec user_prog failed]\n");
    }
    __asm__ volatile ("1:");   // SYS_EXIT returns here

    keyboard_init();
    irq_register_handler(1, keyboard_handler);
    irq_unmask(1);

    vga_putc('\n');
    shell_init();

    for (;;) {}
}
