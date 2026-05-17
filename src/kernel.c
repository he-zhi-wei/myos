#define VGA_BUF   ((volatile char *)0xB8000)
#define SER_PORT  0x3F8

static inline unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void serial_putc(char c) {
    while ((inb(SER_PORT + 5) & 0x20) == 0) {}
    outb(SER_PORT, c);
}

static void serial_puts(const char *s) {
    for (; *s; s++) serial_putc(*s);
}

void kernel_main(unsigned int magic, unsigned int addr) {
    const char *msg = "Hello, OS World!";

    // Write to VGA
    for (int i = 0; msg[i] != '\0'; i++) {
        VGA_BUF[i * 2] = msg[i];
        VGA_BUF[i * 2 + 1] = 0x07;   // light grey on black
    }

    // Write to serial
    serial_puts(msg);
    serial_puts("\n");

    for (;;) {}
}
