// 文件：elf.c
// 作用：ELF32 加载器，把 RamFS 里的可执行文件映射进用户地址空间并以 Ring 3 同步运行。
// 要点：elf_exec() 用 kctx_save/kctx_restore 保存内核上下文，SYS_EXIT 后回到调用者；
//       只接受 ET_EXEC/i386，最多 8 个 PT_LOAD、MAX_ELF_PAGES=64 页；退出时恢复 identity 映射并释放页。
//       不允许嵌套 exec（用户态运行时直接返回 -1）。

#include "elf.h"
#include "paging.h"
#include "pmm.h"
#include "fs.h"
#include "gdt.h"
#include "tss.h"
#include "serial.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

// ── Kernel context saved while Ring 3 runs ──────────
// SYS_EXIT restores this to return into elf_exec() as if it had returned 1.
typedef struct {
    uint32_t esp, ebp, ebx, esi, edi, eip, cs, eflags;
} kctx_t;

extern int  kctx_save(kctx_t *ctx);
extern void kctx_restore(kctx_t *ctx) __attribute__((noreturn));

__asm__ (
".text\n"
".globl kctx_save\n"
".type kctx_save, @function\n"
"kctx_save:\n"
"    movl 4(%esp), %eax\n"        // ctx
"    leal 4(%esp), %ecx\n"        // ESP after a normal ret (arg slot)
"    movl %ecx, 0(%eax)\n"
"    movl %ebp, 4(%eax)\n"
"    movl %ebx, 8(%eax)\n"
"    movl %esi, 12(%eax)\n"
"    movl %edi, 16(%eax)\n"
"    movl (%esp), %ecx\n"         // return address
"    movl %ecx, 20(%eax)\n"
"    movw %cs, %cx\n"
"    movzwl %cx, %ecx\n"
"    movl %ecx, 24(%eax)\n"
"    pushfl\n"
"    popl %ecx\n"
"    movl %ecx, 28(%eax)\n"
"    xorl %eax, %eax\n"
"    ret\n"
".size kctx_save, . - kctx_save\n"
".globl kctx_restore\n"
".type kctx_restore, @function\n"
"kctx_restore:\n"
"    movl 4(%esp), %eax\n"
"    movl 0(%eax), %esp\n"
"    movl 4(%eax), %ebp\n"
"    movl 8(%eax), %ebx\n"
"    movl 12(%eax), %esi\n"
"    movl 16(%eax), %edi\n"
"    pushl 28(%eax)\n"            // eflags
"    pushl 24(%eax)\n"            // cs
"    pushl 20(%eax)\n"            // eip
"    movl $1, %eax\n"             // kctx_save() returns 1 after restore
"    iret\n"
".size kctx_restore, . - kctx_restore\n"
);

static kctx_t exit_ctx;
static int    exit_ctx_valid = 0;
static int    user_mode = 0;

// Pages mapped for the running user image (restored to identity on exit).
#define MAX_ELF_PAGES 64
static uint32_t ldr_vaddr[MAX_ELF_PAGES];
static uint32_t ldr_phys[MAX_ELF_PAGES];
static int      ldr_count = 0;
static uint32_t user_stack_page = 0;
static uint32_t user_kstack_page = 0;

int elf_in_user_mode(void) { return user_mode; }

void elf_exit_to_kernel(void) {
    if (exit_ctx_valid) {
        kctx_restore(&exit_ctx);
        __builtin_unreachable();
    }
}

static void flush_tlb(void) {
    __asm__ volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax", "memory");
}

// Undo the user address-space changes made by elf_exec():
// restore the identity mappings the loader replaced and free the pages.
static void user_pages_release(void) {
    for (int i = 0; i < ldr_count; i++) {
        paging_map_page(ldr_vaddr[i], ldr_vaddr[i], PAGE_PRESENT | PAGE_RW);
        pmm_free_page((void *)ldr_phys[i]);
    }
    ldr_count = 0;

    if (user_stack_page) {
        paging_map_page(user_stack_page, user_stack_page, PAGE_PRESENT | PAGE_RW);
        pmm_free_page((void *)user_stack_page);
        user_stack_page = 0;
    }
    if (user_kstack_page) {
        pmm_free_page((void *)user_kstack_page);
        user_kstack_page = 0;
    }
    tss_set_esp0(0);
    flush_tlb();
}

uint32_t elf_load(const uint8_t *data, uint32_t size) {
    if (size < sizeof(elf32_hdr_t)) return 0;
    elf32_hdr_t *hdr = (elf32_hdr_t *)data;

    if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L'  || hdr->e_ident[3] != 'F')
        return 0;
    if (hdr->e_ident[4] != 1 || hdr->e_ident[5] != 1) return 0;
    if (hdr->e_type != ET_EXEC) return 0;
    if (hdr->e_machine != 3) return 0;

    elf32_phdr_t *phdr = (elf32_phdr_t *)(data + hdr->e_phoff);

    for (uint16_t i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint32_t vaddr  = phdr[i].p_vaddr;
        uint32_t filesz = phdr[i].p_filesz;
        uint32_t memsz  = phdr[i].p_memsz;

        uint32_t flags_pte = PAGE_USER | PAGE_PRESENT;
        if (phdr[i].p_flags & PF_W) flags_pte |= PAGE_RW;

        for (uint32_t off = 0; off < memsz; off += 4096) {
            uint32_t pg_vaddr = (vaddr + off) & 0xFFFFF000;
            uint32_t *page = (uint32_t *)pmm_alloc_page();
            if (!page) return 0;

            uint8_t *dst = (uint8_t *)page;
            for (int j = 0; j < 4096; j++) {
                uint32_t pos = off + j;
                dst[j] = (pos < filesz) ? data[phdr[i].p_offset + pos] : 0;
            }

            paging_map_page(pg_vaddr, (uint32_t)page, flags_pte);
        }
    }

    return hdr->e_entry;
}

// Map one PT_LOAD segment into the current page directory.
static int load_segment(int fd, const elf32_phdr_t *ph) {
    uint32_t seg_off = 0;

    while (seg_off < ph->p_memsz) {
        if (ldr_count >= MAX_ELF_PAGES) return -1;

        void *page = pmm_alloc_page();
        if (!page) return -1;

        uint8_t *dst = (uint8_t *)page;
        for (int j = 0; j < 4096; j++) dst[j] = 0;

        uint32_t vpg     = (ph->p_vaddr + seg_off) & 0xFFFFF000;
        uint32_t page_off = (ph->p_vaddr + seg_off) & 0xFFF;

        if (seg_off < ph->p_filesz) {
            uint32_t chunk = ph->p_filesz - seg_off;
            if (chunk > 4096 - page_off) chunk = 4096 - page_off;

            fs_seek(fd, (int)(ph->p_offset + seg_off), 0);
            int n = fs_read(fd, (char *)dst + page_off, chunk);
            if (n != (int)chunk) { pmm_free_page(page); return -1; }
        }

        uint32_t flags_pte = PAGE_USER | PAGE_PRESENT;
        if (ph->p_flags & PF_W) flags_pte |= PAGE_RW;
        paging_map_page(vpg, (uint32_t)page, flags_pte);

        ldr_vaddr[ldr_count] = vpg;
        ldr_phys[ldr_count]  = (uint32_t)page;
        ldr_count++;

        seg_off += 4096 - page_off;
    }
    return 0;
}

// Load `name` into Ring 3 and run it until SYS_EXIT returns control here.
// Returns 0 after the program exits, -1 if it cannot be loaded.
int elf_exec(const char *name) {
    if (user_mode) return -1;    // no nested exec while a program is running

    int fd = fs_open(name, 0);
    if (fd < 0) return -1;

    // Read ELF header into stack buffer
    elf32_hdr_t hdr;
    int n = fs_read(fd, (char *)&hdr, sizeof(hdr));
    if (n != sizeof(hdr)) { fs_close(fd); return -1; }

    // Verify ELF
    if (hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' ||
        hdr.e_ident[2] != 'L'  || hdr.e_ident[3] != 'F')
        { fs_close(fd); return -1; }
    if (hdr.e_ident[4] != 1 || hdr.e_type != ET_EXEC || hdr.e_machine != 3)
        { fs_close(fd); return -1; }
    if (hdr.e_phnum == 0 || hdr.e_phnum > 8)
        { fs_close(fd); return -1; }

    // Read program headers
    elf32_phdr_t phdrs[8];
    fs_seek(fd, (int)hdr.e_phoff, 0);
    n = fs_read(fd, (char *)phdrs, hdr.e_phnum * sizeof(elf32_phdr_t));
    if (n != (int)(hdr.e_phnum * sizeof(elf32_phdr_t)))
        { fs_close(fd); return -1; }

    ldr_count = 0;
    user_stack_page = 0;
    user_kstack_page = 0;

    // Load each PT_LOAD segment
    for (uint16_t i = 0; i < hdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        if (load_segment(fd, &phdrs[i]) < 0) {
            fs_close(fd);
            user_pages_release();
            return -1;
        }
    }

    fs_close(fd);

    // User stack
    void *ustack = pmm_alloc_page();
    if (!ustack) { user_pages_release(); return -1; }
    user_stack_page = (uint32_t)ustack;
    paging_map_page(user_stack_page, user_stack_page,
                    PAGE_USER | PAGE_PRESENT | PAGE_RW);

    // Kernel stack for TSS (used on every Ring 3 → Ring 0 transition)
    void *kstack = pmm_alloc_page();
    if (!kstack) { user_pages_release(); return -1; }
    user_kstack_page = (uint32_t)kstack;
    tss_set_esp0(user_kstack_page + 4096);

    flush_tlb();

    // Keys typed while the program runs belong to it, not to the shell
    kb_flush();
    shell_input_suspended = 1;
    user_mode = 1;
    exit_ctx_valid = 1;

    if (kctx_save(&exit_ctx) == 0) {
        // First pass: make the user data selector visible to Ring 3
        __asm__ volatile (
            "mov %[sel], %%ax\n"
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"
            :
            : [sel] "i"(GDT_SEL_DATA3)
            : "eax", "memory"
        );

        uint32_t user_sp = user_stack_page + 4096;
        __asm__ volatile (
            "push %[ss]\n"
            "push %[esp]\n"
            "push %[eflags]\n"
            "push %[cs]\n"
            "push %[eip]\n"
            "iret\n"
            :
            : [ss]     "r"(GDT_SEL_DATA3),
              [esp]    "r"(user_sp),
              [eflags] "r"(0x202u),
              [cs]     "r"(GDT_SEL_CODE3),
              [eip]    "r"(hdr.e_entry)
            : "memory"
        );
        __builtin_unreachable();
    }

    // Second pass: SYS_EXIT restored the saved kernel context.
    exit_ctx_valid = 0;
    user_mode = 0;
    shell_input_suspended = 0;
    kb_flush();
    user_pages_release();
    return 0;
}
