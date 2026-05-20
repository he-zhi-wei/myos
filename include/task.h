#pragma once
#include "types.h"

typedef enum { TASK_READY, TASK_RUNNING, TASK_SLEEPING, TASK_DONE } task_state_t;

typedef struct task {
    uint32_t        esp;       // saved stack pointer
    uint32_t        *stack;    // stack base (for free)
    uint32_t        pid;
    char            name[16];
    task_state_t    state;
    uint32_t        wake_at;   // tick when sleep ends
    struct task     *next;
} task_t;

void task_init(void);
task_t *task_create(const char *name, void (*entry)(void));
void task_yield(void);
void task_sleep(uint32_t ms);
void task_exit(void);
void schedule(void);

task_t *task_current(void);
int     task_count(void);
void    task_list(void (*print_fn)(const char *));
