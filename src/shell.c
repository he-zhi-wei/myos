#include "shell.h"
#include "vga.h"
#include "serial.h"

#define MAX_BUF  256
#define MAX_ARGS 16

static char buf[MAX_BUF];
static int  buf_len = 0;

// Helpers that output to both VGA and serial
static void putc2(char c)  { vga_putc(c);  serial_putc(c); }
static void puts2(const char *s) { vga_puts(s); serial_puts(s); }

// --- Command handlers -------------------------------------------------
static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    puts2("  clear   Clear the screen\n");
    puts2("  echo    Print text\n");
    puts2("  help    Show this help\n");
    puts2("  about   Show OS info\n");
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
    puts2("os1 v0.4.0\n");
    puts2("A minimal 32-bit x86 OS kernel\n");
}

// --- Command table ----------------------------------------------------
typedef void (*cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *desc;
    cmd_fn handler;
} cmd_t;

static cmd_t cmds[] = {
    {"clear", "Clear the screen",        cmd_clear},
    {"echo",  "Print text",              cmd_echo},
    {"help",  "Show this help",          cmd_help},
    {"about", "Show OS info",            cmd_about},
};
#define NUM_CMDS (sizeof(cmds) / sizeof(cmd_t))

// Simple string compare (no libc)
static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

// --- Parser + dispatcher -----------------------------------------------
static void exec_cmd(char *input) {
    // Tokenize by spaces
    char *argv[MAX_ARGS];
    int argc = 0;

    while (*input && argc < MAX_ARGS) {
        while (*input == ' ') input++;   // skip spaces
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
