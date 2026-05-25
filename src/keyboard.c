#include "keyboard.h"
#include "port.h"
#include "isr.h"
#include "shell.h"

static int shift = 0;
static int e0_prefix = 0;

// Scancode set 1 → ASCII (no shift)
static const char kbd_us[128] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=',0,
    0,  'q','w','e','r','t','y','u','i','o','p','[',']',0,
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,  0,
    0,  ' ',0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
};

// Scancode set 1 → ASCII (with shift)
static const char kbd_us_shift[128] = {
    0,  0,  '!','@','#','$','%','^','&','*','(',')','_','+',0,
    0,  'Q','W','E','R','T','Y','U','I','O','P','{','}',0,
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',0,
    '|','Z','X','C','V','B','N','M','<','>','?',0,  0,
    0,  ' ',0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
};

void keyboard_handler(registers_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    // 0xE0 has bit7 set; must check before release filter
    if (sc == 0xE0) { e0_prefix = 1; return; }

    // e0_prefix must also be checked before release filter,
    // otherwise E0+0xC8 leaves e0_prefix=1 and eats the next key
    if (e0_prefix) {
        e0_prefix = 0;
        if (sc & 0x80) return;  // extended key release
        if (sc == 0x48)      { shell_input(0x82); return; }
        else if (sc == 0x50) { shell_input(0x83); return; }
        return;
    }

    if (sc & 0x80) {
        if (sc == 0xAA || sc == 0xB6) shift = 0;
        return;
    }

    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }

    if (sc == 0x1C) { shell_input('\n'); kb_putc('\n'); return; }
    if (sc == 0x0E) { shell_input('\b'); return; }

    char c = shift ? kbd_us_shift[sc] : kbd_us[sc];
    if (c) { shell_input((unsigned char)c); kb_putc(c); }
}

// ── Keyboard buffer for user-mode SYS_READ ──────────
#define KB_BUF_SIZE 256
static volatile char kb_buf[KB_BUF_SIZE];
static volatile int  kb_head = 0;  // next write position
static volatile int  kb_tail = 0;  // next read position
static volatile int  kb_count = 0;

void kb_putc(char c) {
    if (kb_count < KB_BUF_SIZE) {
        kb_buf[kb_head] = c;
        kb_head = (kb_head + 1) % KB_BUF_SIZE;
        kb_count++;
    }
}

int kb_getc(void) {
    while (kb_count == 0)
        __asm__ volatile ("sti; hlt; cli");
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    kb_count--;
    return (unsigned char)c;
}

int kb_poll(void) {
    if (kb_count == 0) return -1;
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    kb_count--;
    return (unsigned char)c;
}

void keyboard_init(void) {
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);
    mask &= ~(1 << 2);
    outb(0x21, mask);
}
