// 文件：task.h
// 作用：任务（进程）结构 task_t 与创建、fork、让出、睡眠、调度接口，供内核运行 Ring 0 与 Ring 3 任务。
// 要点：esp 是保存的内核栈指针；user_esp/user_stack 仅 Ring 3 任务有效；kernel_esp0 用于切换 TSS.ESP0；pgdir 为 NULL 表示共享内核页目录。
#pragma once
#include "types.h"

typedef enum { TASK_READY, TASK_RUNNING, TASK_SLEEPING, TASK_DONE } task_state_t;

typedef struct task {
    uint32_t        esp;          // saved kernel stack pointer
    uint32_t        *stack;       // kernel stack base
    uint32_t        user_esp;     // user stack pointer (Ring 3 tasks only)
    uint32_t        *user_stack;  // user stack base (Ring 3 tasks only)
    uint32_t        kernel_esp0;  // top of kernel stack for TSS.ESP0
    uint32_t        *pgdir;       // page directory (NULL = share kernel)
    int             is_user;      // 1 = Ring 3, 0 = Ring 0
    uint32_t        pid;
    char            name[16];
    task_state_t    state;
    uint32_t        wake_at;      // tick when sleep ends
} task_t;

void task_init(void);
task_t *task_create(const char *name, void (*entry)(void));
task_t *task_create_user(const char *name, void (*entry)(void));
int  task_fork(void);            // clone current task, returns PID (0=child)
void task_yield(void);
void task_sleep(uint32_t ms);
void task_exit(void);
void schedule(void);

task_t *task_current(void);
int     task_count_total(void);
void    task_list(void (*print_fn)(const char *));
