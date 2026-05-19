#include "irq.h"
#include "port.h"

static irq_fn handlers[16];

static void pic_remap(void) {
    outb(0x20, 0x11);  outb(0xA0, 0x11);
    outb(0x21, 0x20);  outb(0xA1, 0x28);
    outb(0x21, 0x04);  outb(0xA1, 0x02);
    outb(0x21, 0x01);  outb(0xA1, 0x01);
    outb(0x21, 0xFF);  outb(0xA1, 0xFF);
}

void irq_init(void) {
    for (int i = 0; i < 16; i++) handlers[i] = 0;
    pic_remap();
}

void irq_register_handler(int irq, irq_fn handler) {
    if (irq < 16) handlers[irq] = handler;
}

void irq_unmask(int irq) {
    uint16_t port = 0x21;
    if (irq >= 8) { port = 0xA1; irq -= 8; }
    outb(port, inb(port) & ~(1 << irq));
}

void irq_handler(registers_t *regs) {
    if (regs->int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    int i = regs->int_no - 32;
    if (i < 16 && handlers[i])
        handlers[i](regs);
}
