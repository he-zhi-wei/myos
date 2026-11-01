// 文件：kernel.c
// 作用：内核 C 入口 kernel_main，依次初始化各子系统并进入空闲循环。
// 要点：顺序为 gdt→tss→fs→idt→irq→memory→timer→keyboard→shell；先把 embed.asm 内嵌的 user ELF 写入 RamFS 的 "init"；
//       -DTEST 时先跑 run_all_tests()，最后 sti 开中断并停在本文件的 for(;;) 。

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
        int fd = fs_open("init", 1);
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

    keyboard_init();
    irq_register_handler(1, keyboard_handler);
    irq_unmask(1);

    vga_putc('\n');
    shell_init();

    for (;;) {}
}
