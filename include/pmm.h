// 文件：pmm.h
// 作用：物理内存管理器接口，用位图记录每页占用状态，供分页与任务创建分配/释放物理页。
// 要点：页大小 PAGE_SIZE=4096；pmm_init 在 bitmap_start 处放置位图并登记 total_pages，mark_used/mark_free 按字节区间以页为单位标记。
// 另：alloc 返回页起始物理地址，free 接收同种页地址，记账同时维护空闲页计数。
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
