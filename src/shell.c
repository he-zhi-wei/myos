#include "shell.h"
#include "vga.h"
#include "serial.h"
#include "port.h"
#include "pmm.h"
#include "timer.h"
#include "types.h"

#define MAX_BUF   256
#define MAX_ARGS  16
#define HIST_SIZE 16

static char buf[MAX_BUF];
static int  buf_len = 0;

// Command history (circular buffer)
static char history[HIST_SIZE][MAX_BUF];
static int  hist_idx = 0;
static int  hist_cnt = 0;

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
    puts2("  help     Show this help\n");
    puts2("  about    Show OS info\n");
    puts2("  shutdown Power off the system\n");
}

static void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) putc2(' ');
        puts2(argv[i]);
    }
    putc2('\n');
}

static void cmd_about(int argc, char **argv) {
    (void)argc; (void)argv;
    puts2("os1 v0.6.0\nA minimal 32-bit x86 OS kernel\n");
}

static void cmd_uptime(int argc, char **argv) {
    (void)argc; (void)argv;
    uint32_t s = timer_uptime_sec();
    puts2("Uptime: "); putd2(s);
    puts2("s ("); putd2(timer_ticks()); puts2(" ticks)\n");
}

static uint8_t bcd2bin(uint8_t b) { return (b & 0x0F) + ((b >> 4) * 10); }

static void cmd_date(int argc, char **argv) {
    (void)argc; (void)argv;
    outb(0x70, 0x00); uint8_t sec  = inb(0x71);
    outb(0x70, 0x02); uint8_t min  = inb(0x71);
    outb(0x70, 0x04); uint8_t hour = inb(0x71);
    outb(0x70, 0x07); uint8_t day  = inb(0x71);
    outb(0x70, 0x08); uint8_t mon  = inb(0x71);
    outb(0x70, 0x09); uint8_t year = inb(0x71);

    puts2("20"); putd2(bcd2bin(year)); putc2('-');
    if (bcd2bin(mon) < 10) putc2('0'); putd2(bcd2bin(mon)); putc2('-');
    if (bcd2bin(day) < 10) putc2('0'); putd2(bcd2bin(day)); putc2(' ');
    if (bcd2bin(hour) < 10) putc2('0'); putd2(bcd2bin(hour)); putc2(':');
    if (bcd2bin(min) < 10) putc2('0'); putd2(bcd2bin(min)); putc2(':');
    if (bcd2bin(sec) < 10) putc2('0'); putd2(bcd2bin(sec)); putc2('\n');
}

static uint32_t parse_dec(const char *s) {
    uint32_t v = 0;
    for (; *s; s++) v = v * 10 + (*s - '0');
    return v;
}

static void cmd_sleep(int argc, char **argv) {
    if (argc < 2) { puts2("Usage: sleep <ms>\n"); return; }
    uint32_t ms = parse_dec(argv[1]);
    if (ms == 0) { puts2("0ms\n"); return; }
    uint32_t target = timer_ticks() + (ms * timer_freq() + 999) / 1000;
    while (timer_ticks() < target) { __asm__ volatile ("sti; hlt"); }
    putc2('\n');
}

static void cmd_bench(int argc, char **argv) {
    if (argc < 2) { puts2("Usage: bench <count>\n"); return; }
    uint32_t n = parse_dec(argv[1]);
    if (n == 0) n = 1000000;

    uint32_t t1 = timer_ticks();
    for (volatile uint32_t i = 0; i < n; i++) {}
    uint32_t t2 = timer_ticks();

    puts2("Loops: "); putd2(n); putc2('\n');
    puts2("Ticks: "); putd2(t2 - t1); putc2('\n');
    if (t2 > t1) {
        puts2("Rate:  "); putd2(n / ((t2 - t1) * 10 / timer_freq()));
        puts2(" ops/s (approx)\n");
    }
}

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

static void cmd_history(int argc, char **argv) {
    (void)argc; (void)argv;
    int start = hist_cnt >= HIST_SIZE ? (hist_idx + 1) % HIST_SIZE : 0;
    for (int i = 0; i < hist_cnt && i < HIST_SIZE; i++) {
        int idx = (start + i) % HIST_SIZE;
        puts2(history[idx]); putc2('\n');
    }
}

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

// --- Public API --------------------------------------------------------
void shell_init(void) {
    buf_len = 0;
    shell_prompt();
}

void shell_prompt(void) {
    puts2("os1> ");
}

void shell_input(char c) {
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
