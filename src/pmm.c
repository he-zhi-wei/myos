#include "pmm.h"

// Bitmap: 1 bit per page.  0 = free, 1 = used.
static uint32_t *bitmap = 0;
static uint32_t total_pages = 0;
static uint32_t free_pages = 0;
static uint32_t next_free = 0;

static void bitmap_set(uint32_t idx)   { bitmap[idx / 32] |=  (1 << (idx % 32)); }
static void bitmap_clr(uint32_t idx)   { bitmap[idx / 32] &= ~(1 << (idx % 32)); }
static int  bitmap_test(uint32_t idx)  { return (bitmap[idx / 32] >> (idx % 32)) & 1; }

void pmm_init(uint32_t _total, uint32_t bitmap_start) {
    bitmap = (uint32_t *)bitmap_start;
    total_pages = _total;
    free_pages = total_pages;

    // Start: all pages free
    uint32_t words = (total_pages + 31) / 32;
    for (uint32_t i = 0; i < words; i++)
        bitmap[i] = 0;

    // Mark low 1MB as used (BIOS, VGA buffer, IVT, BDA, etc.)
    pmm_mark_used(0, 0x100000);

    next_free = 0x100000 / PAGE_SIZE;  // start searching above 1MB
}

static void pmm_mark_region(uint32_t start, uint32_t len, int used) {
    uint32_t page = start / PAGE_SIZE;
    uint32_t count = (len + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < count && page + i < total_pages; i++) {
        int was = bitmap_test(page + i);
        if (used && !was) { bitmap_set(page + i); free_pages--; }
        if (!used && was) { bitmap_clr(page + i); free_pages++; }
    }
}

void pmm_mark_free(uint32_t start, uint32_t len) { pmm_mark_region(start, len, 0); }
void pmm_mark_used(uint32_t start, uint32_t len) { pmm_mark_region(start, len, 1); }

void *pmm_alloc_page(void) {
    for (uint32_t i = next_free; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            next_free = i + 1;
            return (void *)(i * PAGE_SIZE);
        }
    }
    for (uint32_t i = 0; i < next_free; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            next_free = i + 1;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free_page(void *addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clr(page);
        free_pages++;
        if (page < next_free) next_free = page;
    }
}

uint32_t pmm_total_pages(void) { return total_pages; }
uint32_t pmm_free_pages(void)  { return free_pages;  }
