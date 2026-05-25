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

// Load an ELF from the RamFS file `name`, set up user stack, iret to entry.
// Never returns on success. Returns -1 if file not found or invalid ELF.
int elf_exec(const char *name);
