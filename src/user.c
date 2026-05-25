// Minimal user-mode program. Compiled as a standalone binary (user.bin)
// and embedded in the kernel. Runs in Ring 3, communicates via int $0x80.

#define SYS_EXIT  0
#define SYS_WRITE 1

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
    return ret;
}

static void write_str(const char *s) {
    int len = 0;
    while (s[len]) len++;
    syscall3(SYS_WRITE, (int)s, len, 0);
}

void _start(void) {
    write_str("Hello from Ring 3!\n");
    syscall3(SYS_EXIT, 0, 0, 0);
    for (;;) {}
}
