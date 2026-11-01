// 文件：syscall.c
// 作用：int 0x80 系统调用分发，按 eax 编号查 table[] 调用对应处理函数（共 17 个）。
// 要点：参数经 ebx/ecx/edx 传入、从 registers_t 读出；SYS_EXIT 调 elf_exit_to_kernel() 回到内核；
//       SYS_FORK 在用户态运行中恒返回 -1（调度器尚不能恢复用户程序），SYS_WAIT 直接返回 0。

#include "isr.h"
#include "vga.h"
#include "serial.h"
#include "keyboard.h"
#include "timer.h"
#include "task.h"
#include "fs.h"
#include "elf.h"

// ── Syscall numbers ──────────────────────────────────
#define SYS_EXIT     0
#define SYS_WRITE    1
#define SYS_READ     2
#define SYS_GETTICKS 3
#define SYS_SLEEP    4
#define SYS_GETPID   5
#define SYS_YIELD    6
#define SYS_UPTIME   7
#define SYS_GETC     8
#define SYS_CLEAR    9
#define SYS_UNAME    10
#define SYS_OPEN     11
#define SYS_CLOSE    12
#define SYS_SEEK     13
#define SYS_EXEC     14
#define SYS_FORK     15
#define SYS_WAIT     16
#define SYS_MAX      17

// ── Handlers ─────────────────────────────────────────

// Terminate the Ring 3 program: restore the kernel context saved by elf_exec()
// (its loader frees the user image and returns to whoever called it).
static void sys_exit(registers_t *r) {
    (void)r;
    elf_exit_to_kernel();
    // No user program was active — nothing to return to.
    for (;;) { __asm__ volatile ("cli; hlt"); }
}

// Write to fd (1=stdout, 2=stderr → VGA+serial; 3+ → file)
static void sys_write(registers_t *r) {
    r->eax = fs_write((int)r->ebx, (const char *)r->ecx, r->edx);
}

// Read from fd (0=stdin → keyboard; 3+ → file)
static void sys_read(registers_t *r) {
    if (r->ebx == 0) {
        // stdin: read with echo (backspace support)
        char *buf = (char *)r->ecx;
        uint32_t max = r->edx;
        if (max == 0) { r->eax = 0; return; }
        uint32_t i = 0;
        while (i < max - 1) {
            int ch = kb_getc();
            if (ch == '\n') { buf[i++] = '\n'; vga_putc('\n'); break; }
            if (ch == '\b') {
                if (i > 0) { i--; vga_putc('\b'); }
                continue;
            }
            buf[i++] = (char)ch;
            vga_putc((char)ch);
        }
        buf[i] = '\0';
        r->eax = i;
    } else {
        r->eax = fs_read((int)r->ebx, (char *)r->ecx, r->edx);
    }
}

static void sys_getticks(registers_t *r) { r->eax = timer_ticks(); }

static void sys_sleep(registers_t *r) {
    uint32_t ms = r->ebx;
    uint32_t target = timer_ticks() + (ms * timer_freq() + 999) / 1000;
    while (timer_ticks() < target)
        __asm__ volatile ("sti; hlt; cli");
    r->eax = 0;
}

static void sys_getpid(registers_t *r) {
    task_t *t = task_current();
    r->eax = t ? t->pid : 0;
}

static void sys_yield(registers_t *r) { (void)r; task_yield(); }

static void sys_uptime(registers_t *r) { r->eax = timer_uptime_sec(); }

static void sys_getc(registers_t *r) {
    int c = kb_poll();
    r->eax = c >= 0 ? (uint32_t)c : 0;
}

static void sys_clear(registers_t *r) { (void)r; vga_clear(); r->eax = 0; }

static void sys_uname(registers_t *r) {
    const char *info = "os1 0.7.0 (i386)\n";
    char *buf = (char *)r->ebx;
    for (int i = 0; info[i]; i++) buf[i] = info[i];
    r->eax = 0;
}

static void sys_open(registers_t *r) {
    const char *name = (const char *)r->ebx;
    int create = (int)r->ecx;   // 0=open existing, 1=create
    r->eax = fs_open(name, create);
}

static void sys_close(registers_t *r) {
    r->eax = fs_close((int)r->ebx);
}

static void sys_seek(registers_t *r) {
    r->eax = fs_seek((int)r->ebx, (int)r->ecx, (int)r->edx);
}

static void sys_exec(registers_t *r) {
    const char *name = (const char *)r->ebx;
    r->eax = elf_exec(name);
}

static void sys_fork(registers_t *r) {
    // The task scheduler cannot resume a running user program yet; report
    // failure instead of corrupting the task table.
    if (elf_in_user_mode()) { r->eax = -1; return; }
    r->eax = task_fork();  // 0 = child, >0 = parent (child PID)
}

static void sys_wait(registers_t *r) {
    (void)r;
    r->eax = 0;
}

// ── Table ────────────────────────────────────────────

typedef void (*syscall_fn)(registers_t *);
static syscall_fn table[SYS_MAX] = {
    [SYS_EXIT]     = sys_exit,
    [SYS_WRITE]    = sys_write,
    [SYS_READ]     = sys_read,
    [SYS_GETTICKS] = sys_getticks,
    [SYS_SLEEP]    = sys_sleep,
    [SYS_GETPID]   = sys_getpid,
    [SYS_YIELD]    = sys_yield,
    [SYS_UPTIME]   = sys_uptime,
    [SYS_GETC]     = sys_getc,
    [SYS_CLEAR]    = sys_clear,
    [SYS_UNAME]    = sys_uname,
    [SYS_OPEN]     = sys_open,
    [SYS_CLOSE]    = sys_close,
    [SYS_SEEK]     = sys_seek,
    [SYS_EXEC]     = sys_exec,
    [SYS_FORK]     = sys_fork,
    [SYS_WAIT]     = sys_wait,
};

void syscall_handler(registers_t *regs) {
    uint32_t num = regs->eax;
    if (num < SYS_MAX && table[num])
        table[num](regs);
    else
        regs->eax = -1;
}
