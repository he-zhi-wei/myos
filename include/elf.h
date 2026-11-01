// 文件：elf.h
// 作用：32 位 ELF 加载器接口，供内核把嵌入的 init.elf 装入内存并在 Ring 3 运行。
// 要点：elf32_hdr_t/elf32_phdr_t 对应 ELF 头与程序头，只处理 PT_LOAD(1) 段，PF_R=4、PF_W=2、PF_X=1；elf_exec 从 RamFS 按名加载，SYS_EXIT 经 elf_exit_to_kernel 回到内核上下文。
// Minimal 32-bit ELF loader.
// Parses ELF header + program headers, loads PT_LOAD segments, sets up
// a user stack, and irets to the entry point.
#pragma once
#include "types.h"

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_hdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

#define PT_LOAD    1
#define PF_X       1
#define PF_W       2
#define PF_R       4
#define ET_EXEC    2

uint32_t elf_load(const uint8_t *data, uint32_t size);

// Load an ELF from the RamFS file `name` and run it in Ring 3.
// Returns 0 once the program exits via SYS_EXIT, -1 if it cannot be loaded.
int elf_exec(const char *name);

// SYS_EXIT: restore the kernel context saved by elf_exec() and return to it.
// Returns immediately if no Ring 3 program is active.
void elf_exit_to_kernel(void);

// 1 while a Ring 3 program is running.
int elf_in_user_mode(void);
