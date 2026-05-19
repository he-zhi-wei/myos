#pragma once
#include "types.h"

#define PAGE_SIZE 4096

void  pmm_init(uint32_t total_pages, uint32_t bitmap_start);
void  pmm_mark_used(uint32_t start, uint32_t len);
void  pmm_mark_free(uint32_t start, uint32_t len);
void* pmm_alloc_page(void);
void  pmm_free_page(void *addr);
uint32_t pmm_total_pages(void);
uint32_t pmm_free_pages(void);
