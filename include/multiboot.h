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
