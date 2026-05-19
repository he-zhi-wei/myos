#include "keyboard.h"
#include "port.h"
#include "isr.h"
#include "vga.h"
#include "serial.h"

static int shift = 0;

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

    if (sc & 0x80) {
        // Key release
        if (sc == 0xAA || sc == 0xB6) shift = 0;
        return;
    }

    // Shift press
    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }

    // Enter
    if (sc == 0x1C) { vga_putc('\n'); serial_putc('\n'); return; }

    // Backspace
    if (sc == 0x0E) { vga_putc('\b'); serial_puts("\b \b"); return; }

    // Lookup ASCII
    char c = shift ? kbd_us_shift[sc] : kbd_us[sc];
    if (c) {
        vga_putc(c);
        serial_putc(c);
    }
}

void keyboard_init(void) {
    // Unmask IRQ1 (keyboard) and IRQ2 (cascade to slave) on master PIC
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);  // clear bit 1 → enable IRQ1
    mask &= ~(1 << 2);  // clear bit 2 → enable IRQ2 (cascade)
    outb(0x21, mask);
}
