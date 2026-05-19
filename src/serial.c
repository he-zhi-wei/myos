#include "port.h"

#define SER_PORT 0x2F8   // COM2

void serial_putc(char c) {
    while ((inb(SER_PORT + 5) & 0x20) == 0) {}
    outb(SER_PORT, c);
}

void serial_puts(const char *s) {
    for (; *s; s++) serial_putc(*s);
}
