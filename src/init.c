// Ring 3 init — simple shell using syscalls.
// Built-in: help, exit, cd (stub), echo.
// Unknown commands: fork+exec("/bin/<name>").

// 文件：init.c
// 作用：被内嵌的 Ring 3 用户 shell（_start 入口），全部功能经 int 0x80 系统调用完成。
// 要点：built-in 命令为 help/exit/echo/clear/about/uptime，其余走 fork + exec("/bin/<cmd>")；
//       但用户态 fork 恒返回 -1、exec 在用户态也会失败，所以外部命令实际不可用；
//       由 Makefile 单独编译成 init.elf，再经 embed.asm 内嵌进内核（不是内核目标文件）。

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

static int sys0(int n) { int r; __asm__("int $0x80":"=a"(r):"a"(n)); return r; }
static int sys1(int n, int a) { int r; __asm__("int $0x80":"=a"(r):"a"(n),"b"(a)); return r; }
static int sys3(int n, int a, int b, int c) {
    int r; __asm__("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}

static void print(const char *s) {
    int len = 0; while (s[len]) len++;
    sys3(SYS_WRITE, FD_STDOUT, (int)s, len);
}
static void putc(char c) { sys3(SYS_WRITE, FD_STDOUT, (int)&c, 1); }
static void read_line(char *buf, int max) { sys3(SYS_READ, FD_STDIN, (int)buf, max); }
static void print_int(int n) {
    char b[12]; int i=10; b[11]=0;
    if(n==0)b[i--]='0';
    while(n>0){b[i--]='0'+n%10;n/=10;}
    print(&b[i+1]);
}

// Built-in: echo args...
static void builtin_echo(char *args) {
    // skip "echo "
    char *s = args;
    while (*s && *s != ' ') s++;
    if (*s) s++;  // skip space
    print(s);
    putc('\n');
}

// Built-in: help
static void builtin_help(void) {
    print("Built-in: echo, help, exit, uptime, clear, about\n");
    print("External (fork+exec): ls, cat, write, edit\n");
}

void _start(void) {
    char buf[256], cmd[256];

    print("os1 usersh — Ring 3 shell (fork+exec)\n");
    print("Type 'help' for commands, 'exit' to quit.\n\n");

    // Test: just print and read once to verify keyboard works
    print("Type something: ");
    read_line(buf, sizeof(buf));
    print("You typed: "); print(buf);

    print("Now entering shell loop...\n");
    for (;;) {
        print("$ ");
        read_line(buf, sizeof(buf));

        int len = 0; while (buf[len] && buf[len] != '\n') len++;
        buf[len] = 0;
        if (len == 0) continue;

        // Extract first word
        int i = 0;
        while (buf[i] && buf[i] != ' ') i++;
        int wlen = i;
        for (int j = 0; j < wlen; j++) cmd[j] = buf[j];
        cmd[wlen] = 0;

        // Built-in commands
        if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == 0) break;
        if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == 0) { builtin_help(); continue; }
        if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == 0) { builtin_echo(buf); continue; }
        if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r') { sys0(SYS_CLEAR); continue; }
        if (cmd[0] == 'a' && cmd[1] == 'b' && cmd[2] == 'o' && cmd[3] == 'u' && cmd[4] == 't') { char ub[64]; sys3(SYS_UNAME,(int)ub,0,0); print(ub); continue; }
        if (cmd[0] == 'u' && cmd[1] == 'p' && cmd[2] == 't' && cmd[3] == 'i' && cmd[4] == 'm' && cmd[5] == 'e') { print_int(sys0(SYS_UPTIME)); putc('\n'); continue; }

        // Try fork+exec("/bin/<cmd>")
        int pid = sys0(SYS_FORK);
        if (pid < 0) { print("fork failed\n"); continue; }
        if (pid == 0) {
            // Child: build path and exec
            char path[64] = "/bin/";
            int p = 5;
            for (int j = 0; j < wlen && p < 60; j++) path[p++] = cmd[j];
            path[p] = 0;
            int rc = sys1(SYS_EXEC, (int)path);
            // If exec fails, print error and exit
            print(cmd); print(": command not found\n");
            sys0(SYS_EXIT);
        } else {
            // Parent: wait for child
            sys1(SYS_WAIT, 0);
        }
    }

    sys0(SYS_EXIT);
    for(;;){}
}
