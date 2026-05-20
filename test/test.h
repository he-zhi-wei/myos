#pragma once
#include "serial.h"
#include "vga.h"

extern int test_passed;
extern int test_failed;

static void put_dec(unsigned int n) {
    if (n >= 10) put_dec(n / 10);
    serial_putc('0' + (n % 10));
}

static void test_result(int ok) {
    if (ok) { test_passed++; serial_puts("  -> PASS\n"); }
    else    { test_failed++; serial_puts("  -> FAIL\n"); }
}
