#include "elf.h"
#include "paging.h"
#include "pmm.h"
#include "fs.h"
#include "gdt.h"
#include "tss.h"
#include "serial.h"
#include "vga.h"

uint32_t elf_load(const uint8_t *data, uint32_t size) {
    if (size < sizeof(elf32_hdr_t)) return 0;
    elf32_hdr_t *hdr = (elf32_hdr_t *)data;

    if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L'  || hdr->e_ident[3] != 'F')
        return 0;
    if (hdr->e_ident[4] != 1 || hdr->e_ident[5] != 1) return 0;
    if (hdr->e_type != ET_EXEC) return 0;
    if (hdr->e_machine != 3) return 0;

    elf32_phdr_t *phdr = (elf32_phdr_t *)(data + hdr->e_phoff);

    for (uint16_t i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint32_t vaddr  = phdr[i].p_vaddr;
        uint32_t filesz = phdr[i].p_filesz;
        uint32_t memsz  = phdr[i].p_memsz;

        uint32_t flags_pte = PAGE_USER | PAGE_PRESENT;
        if (phdr[i].p_flags & PF_W) flags_pte |= PAGE_RW;

        for (uint32_t off = 0; off < memsz; off += 4096) {
            uint32_t pg_vaddr = (vaddr + off) & 0xFFFFF000;
            uint32_t *page = (uint32_t *)pmm_alloc_page();
            if (!page) return 0;

            uint8_t *dst = (uint8_t *)page;
            for (int j = 0; j < 4096; j++) {
                uint32_t pos = off + j;
                dst[j] = (pos < filesz) ? data[phdr[i].p_offset + pos] : 0;
            }

            paging_map_page(pg_vaddr, (uint32_t)page, flags_pte);
        }
    }

    return hdr->e_entry;
}

int elf_exec(const char *name) {
    int fd = fs_open(name, 0);
    if (fd < 0) return -1;

    // Read ELF header into stack buffer
    elf32_hdr_t hdr;
    int n = fs_read(fd, (char *)&hdr, sizeof(hdr));
    if (n != sizeof(hdr)) { fs_close(fd); return -1; }

    // Verify ELF
    if (hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' ||
        hdr.e_ident[2] != 'L'  || hdr.e_ident[3] != 'F')
        { fs_close(fd); return -1; }
    if (hdr.e_ident[4] != 1 || hdr.e_type != ET_EXEC || hdr.e_machine != 3)
        { fs_close(fd); return -1; }

    // Read program headers
    elf32_phdr_t phdrs[8];
    fs_seek(fd, (int)hdr.e_phoff, 0);
    n = fs_read(fd, (char *)phdrs, hdr.e_phnum * sizeof(elf32_phdr_t));
    if (n != (int)(hdr.e_phnum * sizeof(elf32_phdr_t)))
        { fs_close(fd); return -1; }

    // Load each PT_LOAD segment
    for (uint16_t i = 0; i < hdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;

        uint32_t vaddr  = phdrs[i].p_vaddr;
        uint32_t filesz = phdrs[i].p_filesz;
        uint32_t memsz  = phdrs[i].p_memsz;
        uint32_t offset = phdrs[i].p_offset;

        uint32_t flags_pte = PAGE_USER | PAGE_PRESENT;
        if (phdrs[i].p_flags & PF_W) flags_pte |= PAGE_RW;

        for (uint32_t off = 0; off < memsz; off += 4096) {
            uint32_t pg_vaddr = (vaddr + off) & 0xFFFFF000;
            uint32_t *page = (uint32_t *)pmm_alloc_page();
            if (!page) { fs_close(fd); return -1; }

            uint8_t *dst = (uint8_t *)page;
            for (int j = 0; j < 4096; j++) {
                dst[j] = (off + j < filesz) ? 0 : 0;  // filled below
            }

            // Read file data into the page
            if (off < filesz) {
                uint32_t read_sz = filesz - off;
                if (read_sz > 4096) read_sz = 4096;
                fs_seek(fd, (int)(offset + off), 0);
                fs_read(fd, (char *)page, read_sz);
            }

            paging_map_page(pg_vaddr, (uint32_t)page, flags_pte);
        }
    }

    fs_close(fd);

    // User stack
    uint32_t *ustack = (uint32_t *)pmm_alloc_page();
    if (!ustack) return -1;
    paging_map_page((uint32_t)ustack, (uint32_t)ustack,
                    PAGE_USER | PAGE_PRESENT | PAGE_RW);

    // Kernel stack for TSS
    uint32_t *kstack = (uint32_t *)pmm_alloc_page();
    if (!kstack) return -1;
    tss_set_esp0((uint32_t)kstack + 4096);

    // Flush TLB
    __asm__ volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax", "memory");

    // iret to entry point
    uint32_t user_sp = (uint32_t)ustack + 4096;
    __asm__ volatile (
        "push %[ss]\n"
        "push %[esp]\n"
        "push %[eflags]\n"
        "push %[cs]\n"
        "push %[eip]\n"
        "iret\n"
        :
        : [ss]     "r"(GDT_SEL_DATA3),
          [esp]    "r"(user_sp),
          [eflags] "r"(0x202u),
          [cs]     "r"(GDT_SEL_CODE3),
          [eip]    "r"(hdr.e_entry)
        : "memory"
    );
    __builtin_unreachable();
}
