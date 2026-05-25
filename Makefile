ASM := nasm
CC  := gcc
LD  := ld
QEMU := qemu-system-i386

ASFLAGS  := -f elf32
BASE_CFLAGS := -m32 -c -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs -Iinclude
LDFLAGS  := -m elf_i386

SRCDIR  := src
TESTDIR := test

CORE_OBJS := boot.o kernel.o gdt.o idt.o isr.o irq.o serial.o interrupt.o \
             vga.o keyboard.o shell.o pmm.o paging.o memory.o timer.o \
             tss.o task.o syscall.o embed.o fs.o elf.o
TEST_OBJS := test_test.o test_isr.o test_pmm.o test_pit.o test_cmd.o

BIN      := kernel.bin
TEST_BIN := test_kernel.bin
ISO      := kernel.iso

# Default: no tests
OBJS   := $(CORE_OBJS)
CFLAGS := $(BASE_CFLAGS)

.PHONY: all check run test serial iso iso-run kbdtest clean

all: $(BIN)

check: OBJS   := $(CORE_OBJS) $(TEST_OBJS)
check: CFLAGS := $(BASE_CFLAGS) -DTEST -Itest
check:
	@rm -f $(CORE_OBJS) $(TEST_OBJS) $(TEST_BIN)
	@$(MAKE) --no-print-directory _test_build OBJS="$(OBJS)" CFLAGS="$(CFLAGS)"
	@timeout 5 $(QEMU) -kernel $(TEST_BIN) -display none -serial null -serial file:serial.log 2>/dev/null; true
	@echo "=== Test Results ==="
	@tail -3 serial.log
	@rm -f $(CORE_OBJS) $(TEST_OBJS) $(TEST_BIN)
	@rm -f $(TEST_BIN)

_test_build: $(TEST_BIN)

$(TEST_BIN): $(OBJS) linker.ld
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

boot.o: $(SRCDIR)/boot.asm
	@$(ASM) $(ASFLAGS) $< -o $@

interrupt.o: $(SRCDIR)/interrupt.asm
	@$(ASM) $(ASFLAGS) $< -o $@

kernel.o: $(SRCDIR)/kernel.c
	@$(CC) $(CFLAGS) $< -o $@

gdt.o: $(SRCDIR)/gdt.c
	@$(CC) $(CFLAGS) $< -o $@

idt.o: $(SRCDIR)/idt.c
	@$(CC) $(CFLAGS) $< -o $@

isr.o: $(SRCDIR)/isr.c
	@$(CC) $(CFLAGS) $< -o $@

irq.o: $(SRCDIR)/irq.c
	@$(CC) $(CFLAGS) $< -o $@

serial.o: $(SRCDIR)/serial.c
	@$(CC) $(CFLAGS) $< -o $@

test_test.o: $(TESTDIR)/test.c
	@$(CC) $(CFLAGS) -I$(TESTDIR) $< -o $@

test_isr.o: $(TESTDIR)/isr.c
	@$(CC) $(CFLAGS) -I$(TESTDIR) $< -o $@

test_pmm.o: $(TESTDIR)/pmm.c
	@$(CC) $(CFLAGS) -I$(TESTDIR) $< -o $@

test_pit.o: $(TESTDIR)/pit.c
	@$(CC) $(CFLAGS) -I$(TESTDIR) $< -o $@

test_cmd.o: $(TESTDIR)/cmd.c
	@$(CC) $(CFLAGS) -I$(TESTDIR) $< -o $@

vga.o: $(SRCDIR)/vga.c
	@$(CC) $(CFLAGS) $< -o $@

keyboard.o: $(SRCDIR)/keyboard.c
	@$(CC) $(CFLAGS) $< -o $@

shell.o: $(SRCDIR)/shell.c
	@$(CC) $(CFLAGS) $< -o $@

pmm.o: $(SRCDIR)/pmm.c
	@$(CC) $(CFLAGS) $< -o $@

paging.o: $(SRCDIR)/paging.c
	@$(CC) $(CFLAGS) $< -o $@

memory.o: $(SRCDIR)/memory.c
	@$(CC) $(CFLAGS) $< -o $@

timer.o: $(SRCDIR)/timer.c
	@$(CC) $(CFLAGS) $< -o $@
tss.o: $(SRCDIR)/tss.c
	@$(CC) $(CFLAGS) $< -o $@

task.o: $(SRCDIR)/task.c
	@$(CC) $(CFLAGS) $< -o $@

syscall.o: $(SRCDIR)/syscall.c
	@$(CC) $(CFLAGS) $< -o $@
fs.o: $(SRCDIR)/fs.c
	@$(CC) $(CFLAGS) $< -o $@

user.elf: $(SRCDIR)/user.c user.ld
	@$(CC) -m32 -c -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs $(SRCDIR)/user.c -o user.o
	@$(LD) -m elf_i386 -T user.ld -o user.elf user.o
	@rm -f user.o

embed.o: user.elf $(SRCDIR)/embed.asm
	@$(ASM) -f elf32 -I. $(SRCDIR)/embed.asm -o $@
elf.o: $(SRCDIR)/elf.c
	@$(CC) $(CFLAGS) $< -o $@


$(BIN): $(OBJS) linker.ld
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)
	@echo "  BUILD   kernel.bin"

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
	@rm -f user.elf embed.o user.o user.elf
