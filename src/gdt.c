// 文件：gdt.c
// 作用：构建并加载全局描述符表 GDT，提供内核/用户态代码段、数据段与 TSS 描述符。
// 要点：gdt[6] 含 null、CODE0/DATA0、CODE3/DATA3，TSS 槽由 tss_init() 调 gdt_set_tss() 填写；
//       所有段基址为 0、limit 4G（平坦模型），gdt_flush() 用 lgdt + 远跳转重载 CS。

#include "gdt.h"

static gdt_entry_t gdt[6];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[idx].base_low    = base & 0xFFFF;
    gdt[idx].base_mid    = (base >> 16) & 0xFF;
    gdt[idx].base_high   = (base >> 24) & 0xFF;
    gdt[idx].limit_low   = limit & 0xFFFF;
    gdt[idx].granularity = (limit >> 16) & 0x0F;
    gdt[idx].granularity |= gran & 0xF0;
    gdt[idx].access      = access;
}

static void gdt_flush(uint32_t ptr) {
    __asm__ volatile (
        "lgdt (%[p])\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $.L1%=\n"
        ".L1%=:\n"
        :
        : [p] "r"(ptr)
        : "eax", "memory"
    );
}

void gdt_set_tss(uint32_t base, uint32_t limit) {
    // TSS descriptor: system segment, not code/data.
    // access = GDT_ACCESS_TSS (0x89 = P=1 DPL=0 S=0 Type=9)
    // gran = 0 (byte granularity, TSS is never paged)
    gdt_set_entry(GDT_TSS, base, limit, GDT_ACCESS_TSS, 0);
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_entry(GDT_NULL,  0, 0,           0,               0);
    gdt_set_entry(GDT_CODE0, 0, GDT_LIMIT_4G, GDT_ACCESS_CODE0, GDT_GRAN_FLAGS);
    gdt_set_entry(GDT_DATA0, 0, GDT_LIMIT_4G, GDT_ACCESS_DATA0, GDT_GRAN_FLAGS);
    gdt_set_entry(GDT_CODE3, 0, GDT_LIMIT_4G, GDT_ACCESS_CODE3, GDT_GRAN_FLAGS);
    gdt_set_entry(GDT_DATA3, 0, GDT_LIMIT_4G, GDT_ACCESS_DATA3, GDT_GRAN_FLAGS);
    // GDT_TSS is set by tss_init() calling gdt_set_tss()

    gdt_flush((uint32_t)&gdt_ptr);
}
