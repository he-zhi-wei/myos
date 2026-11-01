// 文件：irq.c
// 作用：IRQ 框架，重映射 8259A PIC 并把硬件中断分发到注册的处理函数。
// 要点：IRQ0..15 映射到向量 32..47，handlers[16] 由 irq_register_handler 填充；
//       irq_handler() 先发 EOI（irq>=8 时主/从片都发）再回调，当前只注册了 timer(0) 与 keyboard(1)。

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
