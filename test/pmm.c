#include "test.h"
#include "pmm.h"

void test_pmm_run(void) {
    serial_puts("--- Physical Memory Manager ---\n");

    serial_puts("[PMM] alloc page...");
    void *p1 = pmm_alloc_page();
    test_result(p1 != 0 && (uint32_t)p1 >= 0x100000);

    serial_puts("[PMM] alloc 2nd page...");
    void *p2 = pmm_alloc_page();
    test_result(p2 != 0 && p1 != p2);

    serial_puts("[PMM] free & re-alloc...");
    pmm_free_page(p1);
    void *p3 = pmm_alloc_page();
    test_result(p3 == p1);

    serial_puts("[PMM] free pages > 0...");
    test_result(pmm_free_pages() > 0);

    pmm_free_page(p2);
    pmm_free_page(p3);
}
