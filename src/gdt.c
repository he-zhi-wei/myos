#include "gdt.h"

static gdt_entry_t gdt[3];
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

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_entry(GDT_NULL, 0, 0,           0, 0);
    gdt_set_entry(GDT_CODE, 0, GDT_LIMIT_4G, GDT_ACCESS_CODE, GDT_GRAN_FLAGS);
    gdt_set_entry(GDT_DATA, 0, GDT_LIMIT_4G, GDT_ACCESS_DATA, GDT_GRAN_FLAGS);

    gdt_flush((uint32_t)&gdt_ptr);
}
