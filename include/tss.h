#pragma once
#include "types.h"

// Minimal TSS: only ESP0/SS0 needed for ring transitions
typedef struct {
    uint32_t prev_tss;   // 0x00: backlink
    uint32_t esp0;       // 0x04: kernel stack pointer (Ring 0)
    uint32_t ss0;        // 0x08: kernel stack segment
    uint32_t esp1;       // 0x0C (unused)
    uint32_t ss1;        // 0x10 (unused)
    uint32_t esp2;       // 0x14 (unused)
    uint32_t ss2;        // 0x18 (unused)
    uint32_t cr3;        // 0x1C
    uint32_t eip;        // 0x20
    uint32_t eflags;     // 0x24
    // remaining fields are zeroed and unused
} __attribute__((packed)) tss_t;

void tss_init(void);
void tss_set_esp0(uint32_t esp0);
