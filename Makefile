# ──────────────────────────────────────────────────────────────
# os1 — build and test
#
#   make            build kernel.bin
#   make check      build a -DTEST kernel, boot it headless, verify results
#   make ring3      end-to-end check of the Ring 3 exec/exit chain
#   make run        boot kernel.bin with a QEMU window
#   make serial     boot headless, log COM2 to build/serial.log
#   make kbdtest    type a few keys through the QEMU monitor
#   make iso        build a GRUB iso (needs grub-mkrescue)
#   make clean      remove every generated file
# ──────────────────────────────────────────────────────────────

ASM      ?= nasm
CC       ?= gcc
LD       ?= ld
QEMU     ?= qemu-system-i386

# The IDE runtime ships a libselinux override that qemu's loader chokes on.
# Drop it for every qemu invocation; override with `make check QEMU_ENV=`.
QEMU_ENV ?= env -u LD_LIBRARY_PATH -u LD_PRELOAD

SRCDIR   := src
TESTDIR  := test
BUILD    := build

# release objects, test objects, test/*.c objects
OBJDIR   := $(BUILD)/obj
TOBJDIR  := $(BUILD)/test/obj
UTOBJDIR := $(BUILD)/test/unit

BIN         := kernel.bin
TEST_BIN    := $(BUILD)/test_kernel.bin
TEST_LOG    := $(BUILD)/test.log
SERIAL_LOG  := $(BUILD)/serial.log
RING3_LOG   := $(BUILD)/ring3.log
ISO         := kernel.iso

TEST_TIMEOUT := 10

ASFLAGS     := -f elf32
ARCHFLAGS   := -m32 -ffreestanding -nostdlib -fno-pie -nostartfiles -nodefaultlibs
CFLAGS      := $(ARCHFLAGS) -Wall -Wextra -Iinclude
TEST_CFLAGS := $(CFLAGS) -DTEST -I$(TESTDIR)
DEPFLAGS    := -MMD -MP
LDFLAGS     := -m elf_i386

BASE_OBJS := boot.o interrupt.o kernel.o gdt.o idt.o isr.o irq.o serial.o \
             vga.o keyboard.o shell.o pmm.o paging.o memory.o timer.o \
             tss.o task.o syscall.o fs.o elf.o
OBJS      := $(addprefix $(OBJDIR)/,$(BASE_OBJS)) $(OBJDIR)/embed.o
TEST_CORE := $(addprefix $(TOBJDIR)/,$(BASE_OBJS)) $(TOBJDIR)/embed.o
TEST_UNIT := $(addprefix $(UTOBJDIR)/,test.o isr.o pmm.o pit.o cmd.o)
TEST_OBJS := $(TEST_CORE) $(TEST_UNIT)

.PHONY: all check ring3 run test serial kbdtest iso iso-run clean
.SUFFIXES:
.DELETE_ON_ERROR:

all: $(BIN)

# ── Object rules ───────────────────────────────────────────────
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.asm | $(OBJDIR)
	@$(ASM) $(ASFLAGS) $< -o $@

$(TOBJDIR)/%.o: $(SRCDIR)/%.c | $(TOBJDIR)
	@$(CC) $(TEST_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TOBJDIR)/%.o: $(SRCDIR)/%.asm | $(TOBJDIR)
	@$(ASM) $(ASFLAGS) $< -o $@

$(UTOBJDIR)/%.o: $(TESTDIR)/%.c | $(UTOBJDIR)
	@$(CC) $(TEST_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR) $(TOBJDIR) $(UTOBJDIR) $(BUILD):
	@mkdir -p $@

# ── Embedded Ring 3 program ────────────────────────────────────
init.elf: $(SRCDIR)/init.c user.ld | $(BUILD)
	@$(CC) $(ARCHFLAGS) -c $(SRCDIR)/init.c -o $(BUILD)/init.o
	@$(LD) $(LDFLAGS) -T user.ld -o $@ $(BUILD)/init.o
	@echo "  LD      $@"

$(OBJDIR)/embed.o: init.elf $(SRCDIR)/embed.asm | $(OBJDIR)
	@$(ASM) $(ASFLAGS) -I. $(SRCDIR)/embed.asm -o $@

$(TOBJDIR)/embed.o: init.elf $(SRCDIR)/embed.asm | $(TOBJDIR)
	@$(ASM) $(ASFLAGS) -I. $(SRCDIR)/embed.asm -o $@

# ── Images ─────────────────────────────────────────────────────
$(BIN): $(OBJS) linker.ld
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)
	@echo "  LD      $@"

$(TEST_BIN): $(TEST_OBJS) linker.ld | $(BUILD)
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ $(TEST_OBJS)
	@echo "  LD      $@"

# ── Unit tests ─────────────────────────────────────────────────
# Pass/fail is decided by the markers the kernel prints (see test/test.h);
# a timeout or a crash leaves no marker, which is also a failure.
check: $(TEST_BIN)
	@rm -f $(TEST_LOG)
	@echo "  TEST    booting $(TEST_BIN) (timeout $(TEST_TIMEOUT)s)"
	@timeout $(TEST_TIMEOUT) $(QEMU_ENV) $(QEMU) -kernel $(TEST_BIN) \
		-display none -serial null -serial file:$(TEST_LOG) >/dev/null 2>&1 || true
	@if grep -q '^TESTS_OK' $(TEST_LOG) 2>/dev/null; then \
		sed -n '/====== Unit Tests/,$$p' $(TEST_LOG) | grep -E '^(---|  \[)'; \
		echo "  PASS    $$(grep -h '^RESULT' $(TEST_LOG))"; \
	else \
		echo "  FAIL    unit tests did not pass"; \
		grep -h -E '^(  \[FAIL|TESTS_FAILED|RESULT)' $(TEST_LOG) 2>/dev/null; \
		echo "  ---- serial log ($(TEST_LOG)) ----"; \
		cat $(TEST_LOG) 2>/dev/null || echo "  (no serial output: qemu crashed or timed out)"; \
		exit 1; \
	fi

# ── Ring 3 integration check ───────────────────────────────────
# Drives the real shell through the QEMU monitor: exec init, run one
# command inside the user program, exit, and expect the native shell back.
ring3: $(BIN)
	@rm -f $(RING3_LOG)
	@echo "  TEST    Ring 3 exec/exit chain (timeout 12s)"
	@( sleep 3; \
	  printf 'sendkey e\nsendkey x\nsendkey e\nsendkey c\nsendkey spc\n'; \
	  sleep 0.3; \
	  printf 'sendkey i\nsendkey n\nsendkey i\nsendkey t\nsendkey ret\n'; \
	  sleep 2; \
	  printf 'sendkey h\nsendkey e\nsendkey l\nsendkey l\nsendkey o\nsendkey ret\n'; \
	  sleep 1; \
	  printf 'sendkey e\nsendkey x\nsendkey i\nsendkey t\nsendkey ret\n'; \
	  sleep 2 ) | timeout 12 $(QEMU_ENV) $(QEMU) -kernel $(BIN) \
		-display none -serial null -serial file:$(RING3_LOG) \
		-monitor stdio >/dev/null 2>&1 || true
	@if grep -q 'exec: program exited' $(RING3_LOG) 2>/dev/null; then \
		echo "  PASS    Ring 3 program started and exited"; \
	else \
		echo "  FAIL    Ring 3 program did not exit"; \
		echo "  ---- serial log ($(RING3_LOG)) ----"; \
		cat $(RING3_LOG) 2>/dev/null; \
		exit 1; \
	fi

# ── Interactive targets ────────────────────────────────────────
run: $(BIN)
	$(QEMU_ENV) $(QEMU) -kernel $(BIN)

test: $(BIN)
	$(QEMU_ENV) $(QEMU) -kernel $(BIN) -nographic

serial: $(BIN)
	@mkdir -p $(BUILD)
	$(QEMU_ENV) $(QEMU) -kernel $(BIN) -display none -serial null -serial file:$(SERIAL_LOG)

kbdtest: $(BIN)
	@mkdir -p $(BUILD)
	@rm -f $(SERIAL_LOG)
	@( sleep 3; \
	  printf 'sendkey a\nsendkey b\nsendkey c\nsendkey ret\n'; \
	  sleep 1 ) | \
	timeout 10 $(QEMU_ENV) $(QEMU) -kernel $(BIN) -display none \
		-serial null -serial file:$(SERIAL_LOG) -monitor stdio >/dev/null 2>&1; true
	@echo "=== Keyboard test ($(SERIAL_LOG)) ==="
	@head -n 8 $(SERIAL_LOG) 2>/dev/null; tail -n 3 $(SERIAL_LOG) 2>/dev/null

iso: $(BIN)
	@rm -rf $(BUILD)/iso
	@mkdir -p $(BUILD)/iso/boot/grub
	@cp $(BIN) $(BUILD)/iso/boot/
	@printf 'set timeout=0\nset default=0\nmenuentry "os1" {\n    multiboot /boot/kernel.bin\n    boot\n}\n' \
		> $(BUILD)/iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) $(BUILD)/iso 2>/dev/null
	@echo "  ISO     $@"

iso-run: $(ISO)
	$(QEMU_ENV) $(QEMU) -cdrom $(ISO)

clean:
	@rm -rf $(BUILD) $(BIN) $(ISO) init.elf serial.log $(wildcard *.o)
	@echo "  CLEAN"

# Header dependencies generated by -MMD.
-include $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)
