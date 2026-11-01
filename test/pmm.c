// 文件：test/pmm.c
// 作用：验证物理页分配器的分配、回收与空闲页计数一致性。
// 要点：依赖 pmm_alloc_page/pmm_free_page 等接口，页须 4K 对齐且不低于 1MB。

#include "test.h"
#include "pmm.h"

void test_pmm_run(void) {
    test_section("Physical Memory Manager");

    uint32_t free0 = pmm_free_pages();

    void *p1 = pmm_alloc_page();
    TEST("alloc page is 4K-aligned and above 1MB",
         p1 != 0 && ((uint32_t)p1 & (PAGE_SIZE - 1)) == 0 && (uint32_t)p1 >= 0x100000);

    void *p2 = pmm_alloc_page();
    TEST("second alloc returns a distinct page", p2 != 0 && p2 != p1);

    pmm_free_page(p1);
    void *p3 = pmm_alloc_page();
    TEST("freed page is reused by the next alloc", p3 == p1);

    // Allocate a batch, then release it: the free count must come back.
    void *batch[64];
    int all_ok = 1;
    for (int i = 0; i < 64; i++) {
        batch[i] = pmm_alloc_page();
        if (!batch[i]) all_ok = 0;
    }
    for (int i = 0; i < 64; i++)
        if (batch[i]) pmm_free_page(batch[i]);
    TEST("64 alloc/free cycles conserve the free-page count",
         all_ok && pmm_free_pages() == free0 - 2);   // p2 and p3 are still held

    uint32_t f_before = pmm_free_pages();
    pmm_free_page(p3);
    pmm_free_page(p3);                  // the second free must be a no-op
    TEST("freeing an already-free page is ignored", pmm_free_pages() == f_before + 1);

    void *probe = pmm_alloc_page();
    pmm_free_page(probe);
    uint32_t f2 = pmm_free_pages();
    pmm_mark_used((uint32_t)probe, PAGE_SIZE);
    int used_ok = (pmm_free_pages() == f2 - 1);
    pmm_mark_free((uint32_t)probe, PAGE_SIZE);
    TEST("mark_used / mark_free round-trip", used_ok && pmm_free_pages() == f2);

    TEST("handed-out pages never fall below 1MB",
         (uint32_t)p2 >= 0x100000 && (uint32_t)p3 >= 0x100000);

    pmm_free_page(p2);
    TEST("the test itself leaks no pages", pmm_free_pages() == free0);
}
