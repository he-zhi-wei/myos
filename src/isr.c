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
