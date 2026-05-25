#include "task.h"
#include "gdt.h"
#include "pmm.h"
#include "tss.h"

#define STACK_SIZE  4096
#define MAX_TASKS   8

static task_t tasks[MAX_TASKS];
static int    task_count = 0;
static int    cur_idx = -1;

// Assembly context switch.
// Stack frame (from low=ESP to high):
//   ds(4), edi(4), esi(4), ebp(4), esp(4), ebx(4), edx(4), ecx(4), eax(4),
//   ret(4), err(4), intno(4), eip(4), cs(4), eflags(4)
// For Ring 3 tasks, SS(4) and user_esp(4) are above eflags.
__asm__ (
".globl _task_switch\n"
"_task_switch:\n"
"    pusha\n"
"    push %ds\n"
"    mov 40(%esp), %eax\n"    // old_esp ptr (param 1)
"    mov %esp, (%eax)\n"      // save current esp
"    mov 44(%esp), %esp\n"    // load new esp (param 2)
"    pop %ds\n"
"    popa\n"
"    add $4, %esp\n"          // skip return address
"    add $8, %esp\n"          // skip err_code + int_no
"    iret\n"
);

extern void _task_switch(uint32_t *old_esp, uint32_t new_esp);

task_t *task_create_user(const char *name, void (*entry)(void)) {
    if (task_count >= MAX_TASKS) return 0;

    int idx = task_count++;
    task_t *t = &tasks[idx];

    // Allocate kernel stack
    uint32_t *kstack = (uint32_t *)pmm_alloc_page();
    if (!kstack) { task_count--; return 0; }
    uint32_t *sp = (uint32_t *)((uint32_t)kstack + STACK_SIZE);

    // Allocate user stack
    uint32_t *ustack = (uint32_t *)pmm_alloc_page();
    if (!ustack) { task_count--; return 0; }

    // Build iret frame on kernel stack (high to low):
    //   SS (user data seg)
    //   ESP (user stack top)
    //   EFLAGS
    //   CS (user code seg, RPL=3)
    //   EIP (entry)
    //   int_no (0)
    //   err_code (0)
    //   ret_addr (0)
    //   pusha frame (8 dwords)
    //   ds
    *--sp = GDT_SEL_DATA3;            // ss
    *--sp = (uint32_t)ustack + STACK_SIZE; // esp
    *--sp = 0x202;                    // eflags (IF on, bit1=1)
    *--sp = GDT_SEL_CODE3;            // cs (RPL=3 → iret pops SS+ESP)
    *--sp = (uint32_t)entry;          // eip
    *--sp = 0;                         // int_no
    *--sp = 0;                         // err_code
    *--sp = 0;                         // ret_addr (dummy)
    *--sp = 0; // eax
    *--sp = 0; // ecx
    *--sp = 0; // edx
    *--sp = 0; // ebx
    *--sp = (uint32_t)ustack + STACK_SIZE; // saved esp (original user ESP)
    *--sp = 0; // ebp
    *--sp = 0; // esi
    *--sp = 0; // edi
    *--sp = GDT_SEL_DATA0;            // ds

    t->esp          = (uint32_t)sp;
    t->stack        = kstack;
    t->kernel_esp0  = (uint32_t)kstack + STACK_SIZE;
    t->user_esp     = (uint32_t)ustack + STACK_SIZE;
    t->user_stack   = ustack;
    t->is_user      = 1;
    t->pid          = idx;
    t->state        = TASK_READY;
    t->wake_at      = 0;

    for (int j = 0; j < 15 && name[j]; j++)
        t->name[j] = name[j];
    t->name[15] = '\0';

    return t;
}

void task_init(void) {
    for (int i = 0; i < MAX_TASKS; i++)
        tasks[i].state = TASK_DONE;
}

task_t *task_create(const char *name, void (*entry)(void)) {
    if (task_count >= MAX_TASKS) return 0;

    int idx = task_count++;
    task_t *t = &tasks[idx];

    uint32_t *stack = (uint32_t *)pmm_alloc_page();
    if (!stack) { task_count--; return 0; }

    uint32_t *sp = (uint32_t *)((uint32_t)stack + STACK_SIZE);

    // Build iret frame (high to low on stack)
    *--sp = 0x202;            // eflags (IF on)
    *--sp = 0x08;             // cs
    *--sp = (uint32_t)entry;  // eip
    *--sp = 0;                // int_no
    *--sp = 0;                // err_code
    *--sp = 0;                // return addr (dummy)
    // pusha frame (popa order: edi, esi, ebp, esp, ebx, edx, ecx, eax)
    *--sp = 0;  // eax
    *--sp = 0;  // ecx
    *--sp = 0;  // edx
    *--sp = 0;  // ebx
    *--sp = 0;  // esp (saved)
    *--sp = 0;  // ebp
    *--sp = 0;  // esi
    *--sp = 0;  // edi
    *--sp = 0x10;  // ds

    t->esp          = (uint32_t)sp;
    t->stack         = stack;
    t->kernel_esp0   = (uint32_t)stack + STACK_SIZE;
    t->user_esp      = 0;
    t->user_stack    = 0;
    t->is_user       = 0;
    t->pid           = idx;
    t->state         = TASK_READY;
    t->wake_at       = 0;

    for (int j = 0; j < 15 && name[j]; j++)
        t->name[j] = name[j];
    t->name[15] = '\0';

    return t;
}

void schedule(void) {
    if (task_count == 0) return;

    // Wake sleeping tasks
    extern uint32_t timer_ticks(void);
    uint32_t now = timer_ticks();
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state == TASK_SLEEPING && tasks[i].wake_at <= now)
            tasks[i].state = TASK_READY;
    }

    // Find next ready task (round-robin)
    for (int i = 0; i < task_count; i++) {
        int idx = (cur_idx + 1 + i) % task_count;
        if (tasks[idx].state == TASK_READY) {
            if (cur_idx >= 0 && tasks[cur_idx].state == TASK_RUNNING)
                tasks[cur_idx].state = TASK_READY;
            cur_idx = idx;
            tasks[cur_idx].state = TASK_RUNNING;
            return;
        }
    }
}

void task_yield(void) {
    if (task_count == 0) return;

    uint32_t saved_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(saved_esp));

    int prev = cur_idx;

    // First yield: bootstrap by saving kernel ESP as idle task's context
    if (cur_idx < 0) {
        prev = 0;
        cur_idx = 0;
    }

    tasks[cur_idx].esp = saved_esp;

    schedule();

    if (cur_idx != prev) {
        if (tasks[cur_idx].is_user)
            tss_set_esp0(tasks[cur_idx].kernel_esp0);
        _task_switch(&tasks[prev].esp, tasks[cur_idx].esp);
    }
}

void task_sleep_ms(uint32_t ms) {
    extern uint32_t timer_ticks(void);
    extern uint32_t timer_freq(void);
    if (cur_idx < 0) return;
    tasks[cur_idx].wake_at = timer_ticks() + (ms * timer_freq()) / 1000;
    tasks[cur_idx].state = TASK_SLEEPING;
    task_yield();
}

void task_exit(void) {
    if (cur_idx < 0) return;
    tasks[cur_idx].state = TASK_DONE;
    if (tasks[cur_idx].user_stack)
        pmm_free_page(tasks[cur_idx].user_stack);
    task_count--;
    int prev = cur_idx;
    cur_idx = -1;
    schedule();
    if (cur_idx >= 0 && cur_idx != prev)
        _task_switch(&tasks[prev].esp, tasks[cur_idx].esp);
    for (;;) { __asm__ volatile ("hlt"); }
}

task_t *task_current(void) { return cur_idx >= 0 ? &tasks[cur_idx] : 0; }
int task_count_total(void)  { return task_count; }

void task_list(void (*print_fn)(const char *)) {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state != TASK_DONE) {
            print_fn(tasks[i].name);
            print_fn(" (");
            print_fn(tasks[i].state == TASK_READY   ? "ready" :
                     tasks[i].state == TASK_RUNNING ? "running" : "sleep");
            print_fn(")\n");
        }
    }
}
