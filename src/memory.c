#include "multiboot.h"
#include "pmm.h"
#include "paging.h"
#include "serial.h"

extern uint32_t _kernel_end;

static inline uint32_t page_align_up(uint32_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static void put_dec(uint32_t n) {
    if (n >= 10) put_dec(n / 10);
    serial_putc('0' + n % 10);
}

void memory_init(uint32_t mb_addr) {
    mb_info_t *mb = (mb_info_t *)mb_addr;
    uint32_t total_ram = 0;

    if (mb->flags & (1 << 6)) {
        mb_mmap_entry_t *entry = (mb_mmap_entry_t *)(uint32_t)mb->mmap_addr;
        uint32_t mmap_end = mb->mmap_addr + mb->mmap_length;

        while ((uint32_t)entry < mmap_end) {
            if (entry->type == 1) {
                uint32_t end = entry->base_addr + entry->length;
                if (end > total_ram) total_ram = end;
            }
            entry = (mb_mmap_entry_t *)((uint32_t)entry + entry->size + 4);
        }
    }
    if (total_ram == 0)
        total_ram = (mb->mem_upper + 1024) * 1024;

    uint32_t kernel_end  = (uint32_t)&_kernel_end;
    uint32_t bitmap_addr = page_align_up(kernel_end);
    uint32_t num_pages   = total_ram / PAGE_SIZE;
    uint32_t bitmap_sz   = page_align_up(num_pages / 8);

    pmm_init(num_pages, bitmap_addr);

    // Mark non-RAM high-memory regions as used
    if (mb->flags & (1 << 6)) {
        mb_mmap_entry_t *entry = (mb_mmap_entry_t *)(uint32_t)mb->mmap_addr;
        uint32_t mmap_end = mb->mmap_addr + mb->mmap_length;

        while ((uint32_t)entry < mmap_end) {
            if (entry->type != 1) {
                uint32_t base = entry->base_addr;
                uint32_t len  = entry->length;
                if (base >= 0x100000)
                    pmm_mark_used(base, len);
            }
            entry = (mb_mmap_entry_t *)((uint32_t)entry + entry->size + 4);
        }
    }

    // Mark kernel + bitmap as used
    pmm_mark_used(0x100000, bitmap_addr + bitmap_sz - 0x100000);

    serial_puts("RAM: ");
    put_dec(total_ram / (1024 * 1024));
    serial_puts("MB, ");
    put_dec(pmm_free_pages() * 4 / 1024);
    serial_puts("MB free\n");

    paging_init();
}
