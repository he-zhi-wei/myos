// 文件：tss.h
// 作用：最小 TSS 结构定义与 esp0 设置接口，供 Ring 3 进入 Ring 0 时切换内核栈。
// 要点：从 Ring 3 中断或 int 0x80 进入时 CPU 自动从 TSS 取 ss0:esp0 作为内核栈顶，切换任务前必须调用 tss_set_esp0 指向该任务内核栈顶；实际只用 esp0/ss0，其余字段置零未用。
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
