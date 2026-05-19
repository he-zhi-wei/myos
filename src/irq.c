#include "irq.h"
#include "port.h"
#include "isr.h"

static void pic_remap(void) {
    outb(0x20, 0x11);  // ICW1: init master
    outb(0xA0, 0x11);  // ICW1: init slave
    outb(0x21, 0x20);  // ICW2: master vector offset → 32
    outb(0xA1, 0x28);  // ICW2: slave vector offset → 40
    outb(0x21, 0x04);  // ICW3: master IRQ2 connected to slave
    outb(0xA1, 0x02);  // ICW3: tell slave its cascade
    outb(0x21, 0x01);  // ICW4: 8086 mode
    outb(0xA1, 0x01);  // ICW4: 8086 mode

    // Mask all IRQs for now (0 = enabled, 1 = masked)
    outb(0x21, 0xFF);  // master: mask all
    outb(0xA1, 0xFF);  // slave: mask all
}

void irq_init(void) {
    pic_remap();
}

void irq_handler(registers_t *regs) {
    // Send EOI
    if (regs->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
