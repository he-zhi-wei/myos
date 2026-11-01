// Ring 3 user program — demonstrates syscalls + file I/O.

// 文件：user.c
// 作用：早期的 Ring 3 演示程序（_start 入口），自测 uname、文件 I/O、fork 与 exec 链路。
// 要点：仅通过 int 0x80 访问内核；当前不参与构建（Makefile 只编译 init.c 为 init.elf），仅作参考保留。

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

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

static int sys0(int num) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}
static int sys3(int num, int a, int b, int c) {
    int ret;
    __asm__ volatile ("int $0x80"
        : "=a"(ret) : "a"(num), "b"(a), "c"(b), "d"(c));
    return ret;
}
static int sys1(int num, int a) {
    return sys3(num, a, 0, 0);
}

static void print(const char *s) {
    int len = 0; while (s[len]) len++;
    sys3(SYS_WRITE, FD_STDOUT, (int)s, len);
}
static void read_line(char *buf, int max) {
    sys3(SYS_READ, FD_STDIN, (int)buf, max);
}
static void print_int(int n) {
    char buf[12]; int i = 10; buf[11] = 0;
    if (n == 0) buf[i--] = '0';
    while (n > 0) { buf[i--] = '0' + (n % 10); n /= 10; }
    print(&buf[i + 1]);
}

void _start(void) {
    char buf[128];

    // ── Basic info ─────────────────────────
    sys3(SYS_UNAME, (int)buf, 0, 0);
    print("OS: "); print(buf);

    print("PID: "); print_int(sys0(SYS_GETPID)); print("\n");
    print("Uptime: "); print_int(sys0(SYS_UPTIME)); print("s\n\n");

    // ── File create + write + read ───────
    print("--- File I/O test ---\n");

    int fd = sys3(SYS_OPEN, (int)"test.txt", 1, 0);  // create
    print("open(create) → fd="); print_int(fd); print("\n");

    if (fd >= 0) {
        sys3(SYS_WRITE, fd, (int)"Hello, filesystem!\nAnother line.\n", 33);

        // seek to beginning and read back
        sys3(SYS_SEEK, fd, 0, 0);  // SEEK_SET
        int n = sys3(SYS_READ, fd, (int)buf, sizeof(buf) - 1);
        if (n > 0) { buf[n] = 0; print("content:\n"); print(buf); }

        // seek to offset 7 (skip "Hello, ")
        sys3(SYS_SEEK, fd, 7, 0);
        n = sys3(SYS_READ, fd, (int)buf, sizeof(buf) - 1);
        if (n > 0) { buf[n] = 0; print("from offset 7: "); print(buf); }

        sys3(SYS_CLOSE, fd, 0, 0);
        print("closed.\n\n");
    }

    // ── stdin demo ────────────────────────
    // ── Test SYS_FORK ───────────────────────────────
    print("\n[fork demo] ");
    int pid = sys0(SYS_FORK);
    if (pid < 0) {
        print("fork failed!\n");
    } else if (pid == 0) {
        print("child here! PID="); print_int(sys0(SYS_GETPID));
        print(" parent="); print_int(pid); print("\n");
        // Child: do file I/O with its own fd
        int cfd = sys3(SYS_OPEN, (int)"child.txt", 1, 0);
        if (cfd >= 0) {
            sys3(SYS_WRITE, cfd, (int)"written by child\n", 16);
            sys3(SYS_CLOSE, cfd, 0, 0);
        }
        sys0(SYS_EXIT);
    } else {
        print("parent here! child PID="); print_int(pid);
        print(" my PID="); print_int(sys0(SYS_GETPID)); print("\n");
        sys1(SYS_WAIT, 0);
        print("child done\n");
    }

    // ── Test SYS_EXEC once ───────────────────────────
    print("\n[exec demo]\n");
    int marker = sys3(SYS_OPEN, (int)"_exec_done", 0, 0);
    if (marker < 0) {
        // First run: create marker file, then exec
        sys3(SYS_OPEN, (int)"_exec_done", 1, 0);
        sys1(SYS_EXEC, (int)"user_prog");
        print("[exec failed]\n");
    } else {
        sys3(SYS_CLOSE, marker, 0, 0);
        print("[exec OK - second run]\n");
    }

    print("[done]\n");
    sys0(SYS_EXIT);
    for (;;) {}
}
