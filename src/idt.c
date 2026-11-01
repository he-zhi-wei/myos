// 文件：idt.c
// 作用：构建并加载中断描述符表 IDT，把 32 个异常、16 个 IRQ 和 int 0x80 门挂到汇编桩上。
// 要点：isr0..31 占向量 0..31，irq0..15 占向量 32..47（PIC 重映射后的偏移）；
//       0x80 门使用 IDT_GATE_USER_INT（DPL=3），否则 Ring 3 无法执行 int $0x80。

#include "idt.h"
#include "gdt.h"
#include "port.h"

static idt_entry_t idt[NUM_IDT];
static idt_ptr_t   idt_ptr;

// Assembly stubs declared in interrupt.asm
extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();
extern void syscall_stub();

static void idt_flush(uint32_t ptr) {
    __asm__ volatile ("lidt (%0)" : : "r"(ptr) : "memory");
}

void idt_set_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}

static void (*isr_stubs[NUM_ISR])(void) = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
};

static void (*irq_stubs[NUM_IRQ])(void) = {
    irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
    irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15,
};

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    for (int i = 0; i < NUM_ISR; i++)
        idt_set_entry(i, (uint32_t)isr_stubs[i], GDT_SEL_CODE, IDT_GATE_INT);
    for (int i = 0; i < NUM_IRQ; i++)
        idt_set_entry(i + NUM_ISR, (uint32_t)irq_stubs[i], GDT_SEL_CODE, IDT_GATE_INT);

    // System call gate: DPL=3 so user code can invoke int $0x80
    idt_set_entry(IDT_SYSCALL_VECTOR, (uint32_t)syscall_stub, GDT_SEL_CODE, IDT_GATE_USER_INT);

    idt_flush((uint32_t)&idt_ptr);
}
