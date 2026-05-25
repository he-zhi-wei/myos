#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "serial.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "pmm.h"
#include "paging.h"
#include "timer.h"
#include "tss.h"

#ifdef TEST
#include "test.h"
#endif

void memory_init(uint32_t mb_addr);

// Embedded user program symbols (produced by objcopy)
extern uint8_t _binary_user_bin_start[];
extern uint8_t _binary_user_bin_end[];

// Return context for SYS_EXIT: iret frame (Ring 0, no SS/ESP needed)
uint32_t ret_eip, ret_cs, ret_eflags;

void kernel_main(unsigned int magic, unsigned int addr) {
    vga_clear();
    vga_puts("Hello, OS World!\n");
    serial_puts("Hello, OS World!\n");

    gdt_init();
    tss_init();
    idt_init();
    irq_init();

    memory_init(addr);

    timer_init(100);
    irq_register_handler(0, timer_handler);
    irq_unmask(0);

    __asm__ volatile ("sti");

    serial_puts("Interrupts enabled.\n");
    vga_puts("Interrupts enabled.\n");

#ifdef TEST
    run_all_tests();
#endif

    // Map user program pages (4MB region) with USER flag
    uint32_t user_start = (uint32_t)_binary_user_bin_start;
    uint32_t user_end   = (uint32_t)_binary_user_bin_end;
    uint32_t user_size  = user_end - user_start;

    // Copy user program to 4MB physical
    uint8_t *dst = (uint8_t *)0x400000;
    for (uint32_t i = 0; i < user_size; i++)
        dst[i] = _binary_user_bin_start[i];

    // Map the pages covering the user program with USER + PRESENT + RW
    for (uint32_t off = 0; off < user_size; off += 4096)
        paging_map_page(0x400000 + off, 0x400000 + off,
                        PAGE_USER | PAGE_PRESENT | PAGE_RW);

    {
        char buf[32] = "user_size=";
        int p = 10;
        uint32_t n = user_size;
        if (n >= 100000) { buf[p++] = '0' + n/100000; n %= 100000; }
        if (n >= 10000 || p > 10) { buf[p++] = '0' + n/10000; n %= 10000; }
        buf[p++] = '0' + n/1000; n %= 1000;
        buf[p++] = '0' + n/100; n %= 100;
        buf[p++] = '0' + n/10;
        buf[p++] = '0' + n%10;
        buf[p++] = '\n';
        buf[p] = 0;
        vga_puts(buf);
    }

    if (user_size == 0) {
        vga_puts("SKIPPED user task (empty binary)\n");
        serial_puts("SKIPPED user task (empty binary)\n");
    } else {
    // Launch user task: build an iret frame on a fresh kernel stack,
    // then iret directly to Ring 3. When the user calls SYS_EXIT,
    // syscall_handler pushes the saved return context and iret's back here.
        uint32_t *kstack = (uint32_t *)pmm_alloc_page();
        uint32_t *ustack = (uint32_t *)pmm_alloc_page();
        uint32_t *sp     = (uint32_t *)((uint32_t)kstack + 4096);

        // Map user stack page with USER flag so Ring 3 can access it
        paging_map_page((uint32_t)ustack, (uint32_t)ustack,
                        PAGE_USER | PAGE_PRESENT | PAGE_RW);

        // Build iret frame for Ring 3 entry (high to low)
        *--sp = GDT_SEL_DATA3;                    // ss  (user data)
        *--sp = (uint32_t)ustack + 4096;          // esp (user stack top)
        *--sp = 0x202;                            // eflags (IF on)
        *--sp = GDT_SEL_CODE3;                    // cs  (user code, RPL=3)
        *--sp = 0x400000;                         // eip (user entry)

        tss_set_esp0((uint32_t)kstack + 4096);

        // Save return context and iret to Ring 3.
        // The label "1:" is where SYS_EXIT will return.
        ret_eflags = 0x202;
        ret_cs   = GDT_SEL_CODE0;

        vga_puts("[launching Ring 3]\n");
        serial_puts("[launching Ring 3]\n");

        __asm__ volatile (
            "movl $1f, %[eip]\n"
            "mov %[sp], %%esp\n"
            "iret\n"
            "1:\n"
            : [eip] "=r"(ret_eip)
            : [sp]  "r"(sp)
            : "memory"
        );

        vga_puts("[back in kernel]\n");
        serial_puts("[back in kernel]\n");
    }

    keyboard_init();
    irq_register_handler(1, keyboard_handler);
    irq_unmask(1);

    vga_putc('\n');
    shell_init();

    for (;;) {}
}
