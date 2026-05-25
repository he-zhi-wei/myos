#include "paging.h"
#include "pmm.h"
#include "serial.h"

#define PAGE_DIR   ((uint32_t *)0x200000)   // 2MB — above kernel, below 4MB
#define PAGE_TAB0  ((uint32_t *)0x201000)   // first page table at 2MB+4K
#define PAGE_TAB1  ((uint32_t *)0x202000)   // second at 2MB+8K

void paging_init(void) {
    // Identity-map first 8MB using two page tables (4MB each)
    for (int i = 0; i < 1024; i++) {
        PAGE_TAB0[i] = (i * PAGE_SIZE) | 3;  // present + r/w
    }
    for (int i = 0; i < 1024; i++) {
        PAGE_TAB1[i] = (0x400000 + i * PAGE_SIZE) | 3;
    }

    // Page directory: first two entries point to page tables
    for (int i = 2; i < 1024; i++)
        PAGE_DIR[i] = 0;  // not present
    PAGE_DIR[0] = (uint32_t)PAGE_TAB0 | 3;
    PAGE_DIR[1] = (uint32_t)PAGE_TAB1 | 3;

    // Mark page directory and tables as used in PMM
    pmm_mark_used(0x200000, 0x3000);  // 3 pages: dir + 2 tables

    // Load CR3 and enable paging
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        :
        : "r"((uint32_t)PAGE_DIR)
        : "eax", "memory"
    );

    serial_puts("Paging enabled (8MB identity-mapped).\n");
}

void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pde_idx = virt >> 22;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;

    // Only support first 8MB (2 page tables) for now
    if (pde_idx > 1) return;

    // Ensure PDE has the USER bit if the PTE needs it
    if (flags & PAGE_USER)
        PAGE_DIR[pde_idx] |= PAGE_USER;

    uint32_t *pt = (uint32_t *)(PAGE_DIR[pde_idx] & 0xFFFFF000);
    pt[pte_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF);
}
