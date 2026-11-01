// 文件：test/test.h
// 作用：单元测试框架的公共接口与输出协议声明。
// 要点：输出经 COM2 串口，结果格式被 Makefile 的 make check 解析。

#pragma once
#include "serial.h"

// Unit-test harness.
//
// Every check prints one line to COM2:
//   [PASS] <name>
//   [FAIL] <name> (line N)
// and the run ends with:
//   RESULT <passed>/<total> passed
//   TESTS_OK            (only when failed == 0)
// The Makefile greps for those markers, so a crash, a hang and a failing
// check all turn `make check` into a non-zero exit.
//
// 输出协议（约定固定，勿随意改动）：
// 1. 每个检查输出一行 "  [PASS] <名字>" 或 "  [FAIL] <名字> (line N)"；
// 2. 全部跑完后输出 "RESULT <通过>/<总数> passed"；
// 3. 仅当 failed == 0 时输出 "TESTS_OK"，否则输出 "TESTS_FAILED"；
// 4. Makefile 的 make check 靠 grep 这两个标记判定成败，超时或崩溃（缺 TESTS_OK）也算失败。
extern int test_passed;
extern int test_failed;

void put_dec(unsigned int n);

// Record one check result.  `line` is filled in by the TEST macro.
void test_check_line(const char *name, int ok, int line);

#define TEST(name, cond) test_check_line((name), (cond) != 0, __LINE__)

void test_section(const char *title);

// Prints the summary and the machine-readable markers.  Returns test_failed.
int test_summary(void);

void test_isr_run(void);
void test_pmm_run(void);
void test_pit_run(void);
void test_cmd_run(void);
void run_all_tests(void);
