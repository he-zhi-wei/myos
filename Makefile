ASM := nasm
CC  := gcc
LD  := ld
QEMU := qemu-system-i386

ASFLAGS  := -f elf32
CFLAGS   := -m32 -c -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs -Iinclude
LDFLAGS  := -m elf_i386

SRCDIR  := src
TESTDIR := test

# Core objects (always compiled)
CORE_OBJS := boot.o kernel.o gdt.o idt.o isr.o irq.o serial.o interrupt.o \
             vga.o keyboard.o shell.o pmm.o paging.o memory.o timer.o
# Test objects (debug only)
TEST_OBJS := test_test.o test_isr.o test_pmm.o test_pit.o test_cmd.o
# Full debug build
DBG_OBJS  := $(CORE_OBJS) $(TEST_OBJS)

BIN := kernel.bin
ISO := kernel.iso

.PHONY: all release run test serial iso iso-run kbdtest clean

OBJS := $(DBG_OBJS)

all: CFLAGS += -DTEST -Itest
all: $(BIN)

release:
	@$(MAKE) clean
	@$(MAKE) all OBJS="$(CORE_OBJS)" CFLAGS="$(CFLAGS) -m32 -c -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs -Iinclude"

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

test_test.o: $(TESTDIR)/test.c
	$(CC) $(CFLAGS) $< -o $@

test_isr.o: $(TESTDIR)/isr.c
	$(CC) $(CFLAGS) $< -o $@

test_pmm.o: $(TESTDIR)/pmm.c
	$(CC) $(CFLAGS) $< -o $@

test_pit.o: $(TESTDIR)/pit.c
	$(CC) $(CFLAGS) $< -o $@

test_cmd.o: $(TESTDIR)/cmd.c
	$(CC) $(CFLAGS) $< -o $@

vga.o: $(SRCDIR)/vga.c
	$(CC) $(CFLAGS) $< -o $@

keyboard.o: $(SRCDIR)/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

shell.o: $(SRCDIR)/shell.c
	$(CC) $(CFLAGS) $< -o $@

pmm.o: $(SRCDIR)/pmm.c
	$(CC) $(CFLAGS) $< -o $@

paging.o: $(SRCDIR)/paging.c
	$(CC) $(CFLAGS) $< -o $@

memory.o: $(SRCDIR)/memory.c
	$(CC) $(CFLAGS) $< -o $@

timer.o: $(SRCDIR)/timer.c
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

kbdtest: $(BIN)
	@-rm -f serial.log
	@( sleep 3; \
	  printf 'sendkey a\nsendkey b\nsendkey c\nsendkey ret\n' ; \
	  sleep 1 ) | \
	timeout 8 $(QEMU) -kernel $(BIN) -display none \
	  -serial null -serial file:serial.log -monitor stdio 2>/dev/null; true
	@echo "=== Keyboard test ==="
	@tail -3 serial.log

clean:
	rm -f $(CORE_OBJS) $(TEST_OBJS) $(BIN) $(ISO)
