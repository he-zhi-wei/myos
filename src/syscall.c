#include "isr.h"
#include "vga.h"
#include "serial.h"

#define SYS_EXIT  0
#define SYS_WRITE 1

// Return context saved by kernel_main before launching first Ring 3 task.
// SYS_EXIT pushes eflags/cs/eip and irets back to kernel_main (Ring 0).
extern uint32_t ret_eip, ret_cs, ret_eflags;

void syscall_handler(registers_t *regs) {
    switch (regs->eax) {
    case SYS_EXIT:
        // Build Ring 0 iret frame (no SS/ESP — same privilege)
        __asm__ volatile (
            "push %[eflags]\n"
            "push %[cs]\n"
            "push %[eip]\n"
            "iret\n"
            :
            : [eflags] "r"(ret_eflags),
              [cs]     "r"(ret_cs),
              [eip]    "r"(ret_eip)
            : "memory"
        );
        __builtin_unreachable();

    case SYS_WRITE: {
        const char *s = (const char *)regs->ebx;
        uint32_t len = regs->ecx;
        for (uint32_t i = 0; i < len; i++) {
            vga_putc(s[i]);
            serial_putc(s[i]);
        }
        break;
    }
    }
}
