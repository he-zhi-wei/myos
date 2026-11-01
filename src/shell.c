// 文件：shell.c
// 作用：内核原生 shell（Ring 0），解析命令行并直接调用内核命令表 cmds[] 中的处理函数。
// 要点：命令含 clear/echo/meminfo/uptime/date/sleep/bench/hexdump/history/ls/cat/touch/write/edit/exec/shutdown；
//       支持 16 条环形历史与上下方向键；shell_edit_mode 与 shell_input_suspended 用于把键盘临时让给 edit 或 Ring 3 程序。

#include "shell.h"
#include "vga.h"
#include "serial.h"
#include "port.h"
#include "pmm.h"
#include "timer.h"
#include "types.h"
#include "fs.h"
#include "keyboard.h"
#include "elf.h"

#define MAX_BUF   256
#define MAX_ARGS  16
#define HIST_SIZE 16

static char buf[MAX_BUF];
static int  buf_len = 0;

// Command history (circular buffer)
static char history[HIST_SIZE][MAX_BUF];
static int  hist_idx = 0;
static int  hist_cnt = 0;
static int  hist_pos = -1;      // -1 = not navigating
static char saved_buf[MAX_BUF]; // saved line when entering history
static int  saved_len = 0;

// Helpers
static void putc2(char c)  { vga_putc(c);  serial_putc(c); }
static void puts2(const char *s) { vga_puts(s); serial_puts(s); }
static void putd2(uint32_t n) {
    if (n >= 10) putd2(n / 10);
    putc2('0' + (n % 10));
}
static void putx2(uint32_t n) {
    if (n >= 16) putx2(n / 16);
    uint32_t d = n & 0xF;
    putc2(d < 10 ? '0' + d : 'A' + d - 10);
}
static uint32_t parse_hex(const char *s) {
    uint32_t v = 0;
    for (; *s; s++) {
        char c = *s;
        if (c >= '0' && c <= '9')      v = v * 16 + (c - '0');
        else if (c >= 'a' && c <= 'f') v = v * 16 + (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = v * 16 + (c - 'A' + 10);
        else break;
    }
    return v;
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

// --- Command handlers -------------------------------------------------
static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    puts2("  clear    Clear the screen\n");
    puts2("  echo     Print text\n");
    puts2("  meminfo  Show memory usage\n");
    puts2("  uptime   Show uptime\n");
    puts2("  date     Show date/time\n");
    puts2("  sleep    Sleep N milliseconds\n");
    puts2("  bench    Benchmark N loops\n");
    puts2("  hexdump  Hex dump memory\n");
    puts2("  history  Show command history\n");
    puts2("  ls       List files in RamFS\n");
    puts2("  cat      Print file contents\n");
    puts2("  write    Write text to a file\n");
    puts2("  edit     Edit a file (line mode)\n");
    puts2("  exec     Run an ELF program in Ring 3\n");
    puts2("  touch    Create an empty file\n");
    puts2("  help     Show this help\n");
    puts2("  about    Show OS info\n");
    puts2("  shutdown Power off the system\n");
}
        
// 文件：shell.c
// 作用：内核命令行 shell 的 clear 命令，清空屏幕。
// 要点：无。
static void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
}

// 文件：shell.c
// 作用：内核命令行 shell 的 echo 命令，打印命令行参数。
// 要点：跳过 argv[0]（命令名），用空格分隔参数，最后打印换行符。
static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) putc2(' ');
        puts2(argv[i]);
    }
    putc2('\n');
}

// 文件：shell.c
// 作用：内核命令行 shell 的 about 命令，显示 OS 版本和内核信息。
// 要点：无。
static void cmd_about(int argc, char **argv) {
    (void)argc; (void)argv;
    puts2("os1 v0.6.0\nA minimal 32-bit x86 OS kernel\n");
}

// 文件：shell.c
// 作用：内核命令行 shell 的 uptime 命令，显示系统运行时间（秒数和时钟周期数）。
// 要点：timer_uptime_sec 获取系统运行时间（秒数），timer_ticks 获取当前时间戳（时钟周期数），timer_freq 获取时钟频率。
static void cmd_uptime(int argc, char **argv) {
    (void)argc; (void)argv;
    uint32_t s = timer_uptime_sec();
    puts2("Uptime: "); putd2(s);
    puts2("s ("); putd2(timer_ticks()); puts2(" ticks)\n");
}

static uint8_t bcd2bin(uint8_t b) { return (b & 0x0F) + ((b >> 4) * 10); }

// 文件：shell.c
// 作用：内核命令行 shell 的 date 命令，显示当前日期和时间。
// 要点：从 CMOS 实时时钟读取秒、分、时、日、月、年，转换为 BCD 码后输出。
static void cmd_date(int argc, char **argv) {
    (void)argc; (void)argv;
    outb(0x70, 0x00); uint8_t sec  = inb(0x71);
    outb(0x70, 0x02); uint8_t min  = inb(0x71);
    outb(0x70, 0x04); uint8_t hour = inb(0x71);
    outb(0x70, 0x07); uint8_t day  = inb(0x71);
    outb(0x70, 0x08); uint8_t mon  = inb(0x71);
    outb(0x70, 0x09); uint8_t year = inb(0x71);

    puts2("20"); putd2(bcd2bin(year)); putc2('-');
    if (bcd2bin(mon) < 10)  { putc2('0'); }
    putd2(bcd2bin(mon)); putc2('-');
    if (bcd2bin(day) < 10)  { putc2('0'); }
    putd2(bcd2bin(day)); putc2(' ');
    if (bcd2bin(hour) < 10) { putc2('0'); }
    putd2(bcd2bin(hour)); putc2(':');
    if (bcd2bin(min) < 10)  { putc2('0'); }
    putd2(bcd2bin(min)); putc2(':');
    if (bcd2bin(sec) < 10)  { putc2('0'); }
    putd2(bcd2bin(sec)); putc2('\n');
}

static uint32_t parse_dec(const char *s) {
    uint32_t v = 0;
    for (; *s; s++) {
        if (v > 429496729) break;  // v*10 would overflow uint32_t
        v = v * 10 + (*s - '0');
    }
    return v;
}
// 文件：shell.c
// 作用：内核命令行 shell 的 sleep 命令，使当前进程暂停指定毫秒数。
// 要点：parse_dec 解析命令行参数为十进制整数，timer_ticks 获取当前时间戳，timer_freq 获取时钟频率。
static void cmd_sleep(int argc, char **argv) {
    if (argc < 2) { puts2("Usage: sleep <ms>\n"); return; }
    uint32_t ms = parse_dec(argv[1]);
    if (ms == 0) { puts2("0ms\n"); return; }
    uint32_t target = timer_ticks() + (ms * timer_freq() + 999) / 1000;
    while (timer_ticks() < target) { __asm__ volatile ("sti; hlt"); }
    putc2('\n');
}

// 文件：shell.c
// 作用：内核命令行 shell 的 bench 命令，测试 CPU 性能。
// 要点：parse_dec 解析命令行参数为十进制整数，timer_ticks 获取当前时间戳，timer_freq 获取时钟频率。
static void cmd_bench(int argc, char **argv) {
    if (argc < 2) { puts2("Usage: bench <count>\n"); return; }
    uint32_t n = parse_dec(argv[1]);
    if (n == 0) n = 1000000;

    __asm__ volatile ("sti");  // enable IRQs during bench
    uint32_t t1 = timer_ticks();
    for (volatile uint32_t i = 0; i < n; i++) {}
    uint32_t t2 = timer_ticks();

    puts2("Loops: "); putd2(n); putc2('\n');
    puts2("Ticks: "); putd2(t2 - t1); putc2('\n');
    if (t2 > t1) {
        uint32_t d = t2 - t1;
        uint32_t rate = (n / d) * timer_freq() + ((n % d) * timer_freq()) / d;
        puts2("Rate:  "); putd2(rate);
        puts2(" ops/s\n");
    } else {
        puts2("Rate:  >"); putd2(n * timer_freq());
        puts2(" ops/s (too fast to measure)\n");
    }
}

// 文件：shell.c
// 作用：内核命令行 shell 的 hexdump 命令，以十六进制格式转储内存内容。
// 要点：parse_hex 解析命令行参数为十六进制整数，addr &= ~0xF 对齐到 16 字节边界。
static void cmd_hexdump(int argc, char **argv) {
    uint32_t addr = 0;
    if (argc >= 2) {
        if (argv[1][0] == '0' && (argv[1][1] == 'x' || argv[1][1] == 'X'))
            addr = parse_hex(argv[1] + 2);
        else
            addr = parse_hex(argv[1]);
    }
    addr &= ~0xF;  // align to 16 bytes

    uint8_t *p = (uint8_t *)addr;
    for (int row = 0; row < 16; row++) {
        putx2(addr + row * 16); puts2(": ");
        for (int col = 0; col < 16; col++) {
            putx2(p[row * 16 + col] >> 4);
            putx2(p[row * 16 + col] & 0xF);
            putc2(' ');
        }
        putc2(' ');
        for (int col = 0; col < 16; col++) {
            char c = p[row * 16 + col];
            putc2((c >= 32 && c < 127) ? c : '.');
        }
        putc2('\n');
    }
}

// 文件：shell.c
// 作用：内核命令行 shell 的 history 命令，显示命令历史记录。
// 要点：hist_cnt 为历史记录数量，hist_idx 为当前记录索引，history 为历史记录数组。
static void cmd_history(int argc, char **argv) {
    (void)argc; (void)argv;
    int start = hist_cnt >= HIST_SIZE ? (hist_idx + 1) % HIST_SIZE : 0;
    for (int i = 0; i < hist_cnt && i < HIST_SIZE; i++) {
        int idx = (start + i) % HIST_SIZE;
        puts2(history[idx]); putc2('\n');
    }
}

// 文件：shell.c
// 作用：内核命令行 shell 的 meminfo 命令，显示物理内存信息（总页数、已用页数、空闲页数）。
// 要点：pmm_total_pages 获取总页数，pmm_free_pages 获取空闲页数，计算已用页数。
static void cmd_meminfo(int argc, char **argv) {
    (void)argc; (void)argv;
    uint32_t total = pmm_total_pages();
    uint32_t free  = pmm_free_pages();
    uint32_t used  = total - free;
    puts2("Total: "); putd2(total * 4 / 1024); puts2(" MB (");
    putd2(total);      puts2(" pages)\n");
    puts2("Used : "); putd2(used * 4 / 1024);  puts2(" MB (");
    putd2(used);       puts2(" pages)\n");
    puts2("Free : "); putd2(free * 4 / 1024);  puts2(" MB (");
    putd2(free);       puts2(" pages)\n");
}

static void cmd_shutdown(int argc, char **argv) {
    (void)argc; (void)argv;
    puts2("Shutting down...\n");
    outw(0x604, 0x2000);
    for (;;) { __asm__ volatile ("hlt"); }
}

// 文件：shell.c
// 作用：内核命令行 shell 的初始化、逐字符输入与提示符接口，供键盘中断和主循环驱动。
// 文件：shell.c
// 作用：内核命令行 shell 的 ls 命令，列出当前目录下的文件和子目录。
// 要点：shell_input 接收单个按键而非整行；shell_input_suspended 非 0 表示 Ring 3 程序独占键盘，shell 不得消费按键，退出用户程序后再恢复。
static void cmd_ls(int argc, char **argv) {
    (void)argc; (void)argv;
    fs_list(puts2);
}

// 文件：shell.c
// 作用：执行用户 ELF 程序（Ring 3），加载到内存并跳转执行。
// 要点：argv[1] 为程序路径，elf_exec 返回值为程序退出码；若失败，打印错误信息并返回。
static void cmd_exec(int argc, char **argv) {
    if (argc < 2) { puts2("Usage: exec <file>\n"); return; }
    int rc = elf_exec(argv[1]);
    if (rc < 0) {
        puts2("exec: cannot run ");
        puts2(argv[1]);
        putc2('\n');
        return;
    }
    puts2("exec: program exited with code ");
    putd2(rc);
    putc2('\n');
}

// 文件：shell.c
// 作用：内核命令行 shell 的 cat 命令，读取文件内容并打印到屏幕。
// 要点：argv[1] 为文件名，fs_open 打开文件，fs_read 读取文件内容，fs_close 关闭文件。
static void cmd_cat(int argc, char **argv) {
    if (argc < 2) { puts2("cat: missing filename\n"); return; }
    int fd = fs_open(argv[1], 0);
    if (fd < 0) { puts2("cat: file not found\n"); return; }
    char buf[256];
    int n;
    while ((n = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        puts2(buf);
    }
    fs_close(fd);
}

// Set by cmd_edit to suppress shell_input while edit is active.
int shell_edit_mode = 0;

// Set while a Ring 3 program owns the keyboard (see elf.c).
int shell_input_suspended = 0;

// Read lines until Ctrl+S (0x13) is pressed. Sets shell_input_suspended
// so keyboard_handler doesn't also feed shell_input.
static int shell_edit_lines(char *out, int max) {
    int total = 0;
    out[0] = 0;
    shell_input_suspended = 1;
    kb_flush();  // discard leftover chars from the command line
    char line[256];
    for (;;) {
        int i = 0;
        for (;;) {
            int c = kb_getc();
            if (c == 0x13) { out[total] = 0; shell_input_suspended = 0; return 1; }
            if (c == '\n') break;
            if (c == '\b') {
                if (i > 0) { i--; vga_putc('\b'); }
                continue;
            }
            if (i < (int)sizeof(line) - 1) {
                line[i++] = (char)c;
                vga_putc((char)c);
            }
        }
        line[i] = 0;
        vga_putc('\n');
        int len = 0; while (line[len]) len++;
        if (total + len + 1 >= max) break;
        for (int j = 0; j < len; j++) out[total++] = line[j];
        out[total++] = '\n';
    }
    out[total] = 0;
    shell_input_suspended = 0;
    return 1;
}

// 文件：shell.c
// 作用：内核命令行 shell 的 touch 命令，创建或截断文件（若已存在）。
// 要点：argv[1] 为文件名，fs_open 以写模式打开文件，若成功则关闭文件（截断），并打印成功信息。
static void cmd_touch(int argc, char **argv) {
    if (argc < 2) { puts2("touch: missing filename\n"); return; }
    int fd = fs_open(argv[1], 1);
    if (fd >= 0) fs_close(fd);
    puts2(argv[1]); puts2(" created\n");
}

static void cmd_write(int argc, char **argv) {
    if (argc < 3) { puts2("write: file text...\n"); return; }
    int fd = fs_open(argv[1], 1);
    if (fd < 0) { puts2("write: cannot open file\n"); return; }
    // Build text in buffer, write once
    char buf[512]; int len = 0;
    for (int i = 2; i < argc && len < 500; i++) {
        for (char *p = argv[i]; *p && len < 500; p++) buf[len++] = *p;
        if (i < argc - 1 && len < 500) buf[len++] = ' ';
    }
    buf[len++] = '\n';
    fs_write(fd, buf, len);
    fs_close(fd);
    puts2("written.\n");
}

// 文件：shell.c
// 作用：内核命令行 shell 的编辑功能，允许用户编辑文件内容。
// 要点：编辑模式下，shell_input 不消费按键，而是把它们直接输出到 VGA 并保存到缓冲区；
//       编辑完成后，Ctrl+S 保存文件并退出编辑模式。
static void cmd_edit(int argc, char **argv) {
    if (argc < 2) { puts2("edit: missing filename\n"); return; }

    vga_putc('\n');  // start on a fresh line
    int fd = fs_open(argv[1], 0);
    if (fd >= 0) {
        puts2("--- current content ---\n");
        char buf[256];
        int n;
        while ((n = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = 0; puts2(buf);
        }
        fs_close(fd);
    }
    puts2("--- enter text, Ctrl+S to save ---\n");

    char new_content[4096];
    shell_edit_lines(new_content, sizeof(new_content));

    int len = 0; while (new_content[len]) len++;

    fd = fs_open(argv[1], 1);
    if (fd < 0) { puts2("edit: cannot open file\n"); return; }
    fs_write(fd, new_content, len);
    fs_close(fd);
    puts2("saved.\n");
}

// --- Command table ----------------------------------------------------
typedef void (*cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *desc;
    cmd_fn handler;
} cmd_t;

static cmd_t cmds[] = {
    {"clear",    "Clear the screen",        cmd_clear},
    {"echo",     "Print text",              cmd_echo},
    {"meminfo",  "Show memory usage",       cmd_meminfo},
    {"uptime",   "Show uptime",             cmd_uptime},
    {"date",     "Show date/time",          cmd_date},
    {"sleep",    "Sleep N milliseconds",    cmd_sleep},
    {"bench",    "Benchmark N loops",       cmd_bench},
    {"hexdump",  "Hex dump memory",         cmd_hexdump},
    {"history",  "Show command history",    cmd_history},
    {"help",     "Show this help",          cmd_help},
    {"about",    "Show OS info",            cmd_about},
    {"ls",       "List files in RamFS",     cmd_ls},
    {"cat",      "Print file contents",     cmd_cat},
    {"touch",    "Create an empty file",    cmd_touch},
    {"write",    "Write text to a file",    cmd_write},
    {"edit",     "Edit a file (line mode)", cmd_edit},
    {"exec",     "Run an ELF program",      cmd_exec},
    {"shutdown", "Power off the system",    cmd_shutdown},
};
#define NUM_CMDS (sizeof(cmds) / sizeof(cmd_t))

// --- History -----------------------------------------------------------
static void hist_add(const char *cmd) {
    // Copy to circular buffer
    char *dst = history[hist_idx];
    int i = 0;
    for (; i < MAX_BUF - 1 && cmd[i]; i++) dst[i] = cmd[i];
    dst[i] = '\0';
    hist_idx = (hist_idx + 1) % HIST_SIZE;
    if (hist_cnt < HIST_SIZE) hist_cnt++;
}

// --- Parser + dispatcher -----------------------------------------------
static void exec_cmd(char *input) {
    char *argv[MAX_ARGS];
    int argc = 0;

    while (*input && argc < MAX_ARGS) {
        while (*input == ' ') input++;
        if (!*input) break;
        argv[argc++] = input;
        while (*input && *input != ' ') input++;
        if (*input) { *input = '\0'; input++; }
    }

    if (argc == 0) return;

    for (unsigned int i = 0; i < NUM_CMDS; i++) {
        if (streq(argv[0], cmds[i].name)) {
            cmds[i].handler(argc, argv);
            return;
        }
    }
    puts2("unknown command: ");
    puts2(argv[0]);
    putc2('\n');
}

// Return pointer to the n-th most recent history entry (0 = latest)
static const char *hist_get(int n) {
    if (n >= hist_cnt) return 0;
    if (n >= HIST_SIZE) return 0;
    int idx = (hist_idx - 1 - n + HIST_SIZE * 2) % HIST_SIZE;
    return history[idx];
}

// Erase current line and redraw with buf content
static void line_redraw(void) {
    int y = vga_get_cursor_y();
    vga_set_cursor(0, y);
    vga_puts("os1> ");
    for (int i = 0; i < buf_len; i++) vga_putc(buf[i]);
    vga_clear_line(y, 5 + buf_len);
    vga_set_cursor(5 + buf_len, y);
}

static void hist_load(const char *s) {
    buf_len = 0;
    while (*s && buf_len < MAX_BUF - 1) buf[buf_len++] = *s++;
    buf[buf_len] = '\0';
    line_redraw();
}

// 文件：shell.c
// 作用：内核命令行 shell 的初始化、逐字符输入与提示符接口，供键盘中断和主循环驱动。
// 要点：shell_input 接收单个按键而非整行；shell_input_suspended 非 0 表示 Ring 3 程序独占键盘，shell 不得消费按键，退出用户程序后再恢复。
//  --- Public API --------------------------------------------------------
void shell_init(void) {
    buf_len = 0;
    hist_pos = -1;
    shell_prompt();
}

void shell_prompt(void) {
    puts2("os1> ");
}

void shell_input(int c) {
    // Up arrow: previous history
    if (c == 0x82) {
        if (hist_pos == -1) {
            for (int i = 0; i < buf_len; i++) saved_buf[i] = buf[i];
            saved_len = buf_len;
            hist_pos = 0;
        } else {
            hist_pos++;
        }
        const char *h = hist_get(hist_pos);
        if (h) hist_load(h);
        else hist_pos--;
        return;
    }

    // Down arrow: next history
    if (c == 0x83) {
        if (hist_pos > 0) {
            hist_pos--;
            hist_load(hist_get(hist_pos));
        } else if (hist_pos == 0) {
            // Back to saved line
            hist_pos = -1;
            buf_len = 0;
            for (int i = 0; i < saved_len; i++) buf[buf_len++] = saved_buf[i];
            line_redraw();
        }
        return;
    }

    // Other input resets history browsing
    hist_pos = -1;

    if (c == '\n') {
        putc2('\n');
        if (buf_len > 0) {
            buf[buf_len] = '\0';
            hist_add(buf);
            exec_cmd(buf);
            buf_len = 0;
        }
        shell_prompt();
    } else if (c == '\b') {
        if (buf_len > 0) {
            buf_len--;
            vga_putc('\b');
        }
    } else {
        if (buf_len < MAX_BUF - 1) {
            buf[buf_len++] = c;
            putc2(c);
        }
    }
}
