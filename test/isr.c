// 文件：test/isr.c
// 作用：验证异常与 IRQ 进入处理器后能正确返回调用者。
// 要点：用 int $N 软中断触发，依赖 IDT 及 irq_handler 的 EOI 处理。

#include "test.h"

// Execute a trap/IRQ and check that control returns to the caller.
// The handler prints the exception line on its way through, so the check
// itself is recorded after the instruction has returned.
#define TRAP(name, insn) do { __asm__ volatile (insn); TEST(name, 1); } while (0)

void test_isr_run(void) {
    test_section("ISR trap path (handler returns to caller)");
    TRAP("int $0x00 #DE divide error",   "int $0");
    TRAP("int $0x01 #DB debug",          "int $1");
    TRAP("int $0x03 #BP breakpoint",     "int $3");
    TRAP("int $0x04 #OF overflow",       "int $4");
    TRAP("int $0x06 #UD invalid opcode", "int $6");
    TRAP("int $0x07 #NM device missing", "int $7");

    test_section("IRQ path (asm stub + irq_handler + EOI)");
    TRAP("IRQ0  timer line",              "int $32");
    TRAP("IRQ1  keyboard line",           "int $33");
    TRAP("IRQ2  cascade line",            "int $34");
    TRAP("IRQ7  spurious line",           "int $39");
    TRAP("IRQ15 slave-PIC line",          "int $47");
}
