ASM := nasm
CC  := gcc
LD  := ld
QEMU := qemu-system-i386

ASFLAGS  := -f elf32
CFLAGS   := -m32 -c -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs -Iinclude
LDFLAGS  := -m elf_i386

SRCDIR := src
OBJS   := boot.o kernel.o gdt.o idt.o isr.o irq.o serial.o interrupt.o
BIN    := kernel.bin
ISO    := kernel.iso

.PHONY: all run test serial iso iso-run clean

all: $(BIN)

boot.o: $(SRCDIR)/boot.asm
	$(ASM) $(ASFLAGS) $< -o $@

interrupt.o: $(SRCDIR)/interrupt.asm
	$(ASM) $(ASFLAGS) $< -o $@

kernel.o: $(SRCDIR)/kernel.c
	$(CC) $(CFLAGS) $< -o $@

gdt.o: $(SRCDIR)/gdt.c
	$(CC) $(CFLAGS) $< -o $@

idt.o: $(SRCDIR)/idt.c
	$(CC) $(CFLAGS) $< -o $@

isr.o: $(SRCDIR)/isr.c
	$(CC) $(CFLAGS) $< -o $@

irq.o: $(SRCDIR)/irq.c
	$(CC) $(CFLAGS) $< -o $@

serial.o: $(SRCDIR)/serial.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

run: $(BIN)
	$(QEMU) -kernel $(BIN)

test: $(BIN)
	$(QEMU) -kernel $(BIN) -nographic

serial: $(BIN)
	$(QEMU) -kernel $(BIN) -display none -serial null -serial file:serial.log

iso: $(BIN)
	mkdir -p iso/boot/grub
	cp $(BIN) iso/boot/
	echo 'set timeout=0'   >  iso/boot/grub/grub.cfg
	echo 'set default=0'   >> iso/boot/grub/grub.cfg
	echo 'menuentry "os1" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	echo '    boot'         >> iso/boot/grub/grub.cfg
	echo '}'                >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso 2>/dev/null
	rm -rf iso

iso-run: $(ISO)
	$(QEMU) -cdrom $(ISO)

clean:
	rm -f $(OBJS) $(BIN) $(ISO)
