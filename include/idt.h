#pragma once
#include "types.h"

// ── IDT entry counts ────────────────────────────────
#define NUM_ISR 32
#define NUM_IRQ 16
#define NUM_IDT 256

// ── IDT gate flag bits ──────────────────────────────
#define IDT_FLAGS_PRESENT 0x80   // gate present
#define IDT_FLAGS_DPL0    0x00   // ring 0
#define IDT_FLAGS_DPL3    0x60   // ring 3
#define IDT_FLAGS_INT32   0x0E   // 32-bit interrupt gate (clears IF on entry)
#define IDT_FLAGS_TRAP32  0x0F   // 32-bit trap gate (does not clear IF)

// ── Prebuilt gate flags ─────────────────────────────
#define IDT_GATE_INT       (IDT_FLAGS_PRESENT | IDT_FLAGS_DPL0 | IDT_FLAGS_INT32)   // 0x8E
#define IDT_GATE_TRAP      (IDT_FLAGS_PRESENT | IDT_FLAGS_DPL0 | IDT_FLAGS_TRAP32)  // 0x8F
#define IDT_GATE_USER_INT  (IDT_FLAGS_PRESENT | IDT_FLAGS_DPL3 | IDT_FLAGS_INT32)   // 0xEE (DPL=3)
#define IDT_SYSCALL_VECTOR 0x80

typedef struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

void idt_init(void);
void idt_set_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
