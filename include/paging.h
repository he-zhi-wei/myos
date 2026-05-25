#pragma once
#include "types.h"

// Page table entry flags
#define PAGE_PRESENT  1
#define PAGE_RW       2
#define PAGE_USER     4

void paging_init(void);
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
