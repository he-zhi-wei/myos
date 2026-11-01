// 文件：isr.c
// 作用：CPU 异常处理入口，并负责把 int 0x80 转交给系统调用分发器。
// 要点：int_no==0x80 时调用 syscall_handler；其余按 exceptions[32] 表向 COM2 打印异常名；
//       只有双故障(8)与机器检查(18)死循环挂起，其他异常打印后返回继续执行。

#include "isr.h"
#include "serial.h"

static const char *exceptions[] = {
    "Division by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU error",
    "Alignment check",
    "Machine check",
    "SIMD floating-point",
    "Virtualisation",
    "Control protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor injection",
    "VMM communication",
    "Security exception",
    "Reserved",
};

void isr_handler(registers_t *regs) {
    // System call dispatch
    if (regs->int_no == 0x80) {
        extern void syscall_handler(registers_t *);
        syscall_handler(regs);
        return;
    }

    serial_puts("  -> CPU Exception #");
    if (regs->int_no >= 10) serial_putc('0' + regs->int_no / 10);
    serial_putc('0' + regs->int_no % 10);
    serial_puts(": ");
    serial_puts(exceptions[regs->int_no]);
    serial_puts("\n");

    // Halt on truly fatal exceptions; return for others
    if (regs->int_no == 8 || regs->int_no == 18) {
        serial_puts("  Fatal, halting.\n");
        for (;;) {}
    }
}
