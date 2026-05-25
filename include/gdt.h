#pragma once
#include "types.h"

// ── GDT selector indices ────────────────────────────
#define GDT_NULL    0
#define GDT_CODE0   1
#define GDT_DATA0   2
#define GDT_CODE3   3
#define GDT_DATA3   4
#define GDT_TSS     5

// ── Backward-compat aliases ──────────────────────────
#define GDT_CODE GDT_CODE0
#define GDT_DATA GDT_DATA0

// ── Segment selectors (index | TI | RPL) ────────────
#define GDT_SEL_CODE0 (GDT_CODE0 << 3)           // 0x08
#define GDT_SEL_DATA0 (GDT_DATA0 << 3)           // 0x10
#define GDT_SEL_CODE3 ((GDT_CODE3 << 3) | 3)     // 0x1B
#define GDT_SEL_DATA3 ((GDT_DATA3 << 3) | 3)     // 0x23
#define GDT_SEL_TSS   (GDT_TSS << 3)             // 0x28

// ── Backward-compat aliases ──────────────────────────
#define GDT_SEL_CODE GDT_SEL_CODE0
#define GDT_SEL_DATA GDT_SEL_DATA0

// ── Access byte fields ──────────────────────────────
#define GDT_ACCESS_PRESENT  0x80   // segment present
#define GDT_ACCESS_DPL0     0x00   // ring 0
#define GDT_ACCESS_DPL3     0x60   // ring 3
#define GDT_ACCESS_CODE_DATA 0x10   // code/data (not system) descriptor
#define GDT_ACCESS_EXEC     0x08   // executable
#define GDT_ACCESS_READ     0x02   // readable (code)
#define GDT_ACCESS_WRITE    0x02   // writable (data)

// ── Prebuilt access bytes ───────────────────────────
#define GDT_ACCESS_CODE0 (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 |       \
                          GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXEC |     \
                          GDT_ACCESS_READ)             // 0x9A

#define GDT_ACCESS_DATA0 (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 |       \
                          GDT_ACCESS_CODE_DATA | GDT_ACCESS_WRITE)     // 0x92

#define GDT_ACCESS_CODE3 (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 |       \
                          GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXEC |     \
                          GDT_ACCESS_READ)             // 0xFA

#define GDT_ACCESS_DATA3 (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 |       \
                          GDT_ACCESS_CODE_DATA | GDT_ACCESS_WRITE)     // 0xF2

#define GDT_ACCESS_TSS   0x89  // P=1 DPL=0 S=0 Type=1001 (32-bit available TSS)

// ── Backward-compat aliases ──────────────────────────
#define GDT_ACCESS_CODE GDT_ACCESS_CODE0
#define GDT_ACCESS_DATA GDT_ACCESS_DATA0

// ── Granularity flags (high nibble) ─────────────────
#define GDT_GRAN_4K    0x80   // 4 KB pages
#define GDT_GRAN_32BIT  0x40   // 32-bit default ops
#define GDT_GRAN_FLAGS (GDT_GRAN_4K | GDT_GRAN_32BIT)  // 0xC0

// ── Limit ───────────────────────────────────────────
#define GDT_LIMIT_4G  0xFFFFFFFF

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

void gdt_init(void);
void gdt_set_tss(uint32_t base, uint32_t limit);
