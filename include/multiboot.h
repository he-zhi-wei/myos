// 文件：multiboot.h
// 作用：Multiboot 1 信息结构定义，供内核解析 boot.asm 通过 EBX 传入的内存布局。
// 要点：mb_info_t 的字段仅在 mbi->flags 对应位为 1 时有效；mmap_length/mmap_addr 指向 mb_mmap_entry_t 数组（含 size/base_addr/length/type，type=1 表示可用 RAM），两个结构均为 packed。
#pragma once
#include "types.h"

typedef struct {
    uint32_t size;          // entry size (>= 24)
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;          // 1 = available RAM
} __attribute__((packed)) mb_mmap_entry_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} mb_info_t;
