// 文件：tss.c
// 作用：TSS 初始化，为 Ring 3 → Ring 0 的中断与系统调用切换提供内核栈。
// 要点：TSS 描述符由 gdt_set_tss() 填入 GDT 的 GDT_TSS 槽，随后 ltr 加载；esp0 由 elf_exec/task 通过 tss_set_esp0 按需设置。

#include "tss.h"
#include "gdt.h"

static tss_t tss;

void tss_init(void) {
    // Zero entire TSS
    uint8_t *p = (uint8_t *)&tss;
    for (unsigned i = 0; i < sizeof(tss); i++)
        p[i] = 0;

    tss.ss0  = GDT_SEL_DATA;          // kernel data segment
    tss.esp0 = 0;                      // set per-task later

    extern void gdt_set_tss(uint32_t base, uint32_t limit);
    gdt_set_tss((uint32_t)&tss, sizeof(tss) - 1);

    // Load task register
    __asm__ volatile ("ltr %%ax" : : "a"(GDT_SEL_TSS) : "memory");
}

void tss_set_esp0(uint32_t esp0) {
    tss.esp0 = esp0;
}
