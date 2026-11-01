// 文件：paging.h
// 作用：32 位两级页表的初始化、映射与页目录切换接口，供内核及 Ring 3 任务管理地址空间。
// 要点：页表项标志 PAGE_PRESENT=1、PAGE_RW=2、PAGE_USER=4，用户态可访问的页必须同时置 PRESENT|USER（否则 Ring 3 访问触发页错误），可写还要加 PAGE_RW。
// 另：paging_clone_dir 复制页目录用于 fork 的用户空间，paging_switch_dir 通过写 CR3 生效且需 TLB 刷新。
#pragma once
#include "types.h"

// Page table entry flags
#define PAGE_PRESENT  1
#define PAGE_RW       2
#define PAGE_USER     4

void paging_init(void);
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
uint32_t *paging_clone_dir(void);
void paging_switch_dir(uint32_t *pgdir);
