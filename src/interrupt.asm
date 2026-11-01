; 文件：interrupt.asm
; 作用：提供 32 个异常 stub、16 个 IRQ stub 及统一的寄存器保存/恢复流程，转交 C 侧 isr_handler、irq_handler 与系统调用处理。
; 要点：CPU 压错误码的异常走 ISR_ERR、其余补 push 0，保证栈帧统一；registers_t 字段顺序依赖此处 DS → pusha → int_no/err_code 的压栈次序，不得随意调整。
; 另：IRQ n 对应 IDT 向量 32+n，int 0x80 的 syscall_stub 复用 isr_common；isr_common/irq_common 返回前用 add esp,8 丢弃 err_code 与 int_no 再 iret。
; ISR stub — no error code pushed by CPU
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push 0          ; dummy error code for uniform stack frame
    push %1         ; interrupt number
    jmp isr_common
%endmacro

; ISR stub — CPU already pushed an error code
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push %1         ; interrupt number
    jmp isr_common
%endmacro

; IRQ stub
%macro IRQ 2
global irq%1
irq%1:
    cli
    push 0          ; dummy error code
    push %2         ; interrupt number (32 + irq_num)
    jmp irq_common
%endmacro

; --- ISRs 0–31 ---
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8        ; double fault (CPU pushes error code)
ISR_NOERR 9
ISR_ERR   10       ; invalid TSS
ISR_ERR   11       ; segment not present
ISR_ERR   12       ; stack-segment fault
ISR_ERR   13       ; general protection fault
ISR_ERR   14       ; page fault
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17       ; alignment check
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30       ; security exception
ISR_NOERR 31

; --- IRQs 0–15 mapped to IDT 32–47 ---
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; --- Common ISR handler ---
isr_common:
    pusha               ; save EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    mov ax, ds
    push eax            ; save DS
    mov ax, 0x10        ; GDT_SEL_DATA: kernel data segment (index 2, TI=0, RPL=0)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pass pointer to registers_t
    extern isr_handler
    call isr_handler
    add esp, 4

    pop eax             ; restore DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                ; restore general registers
    add esp, 8          ; clean up err_code and int_no
    iret

; ── System call stub (int 0x80, DPL=3) ──────────────
; Entered from Ring 3: CPU has already pushed SS, ESP, EFLAGS, CS, EIP.
; We push dummy err_code and int_no, then reuse isr_common → isr_handler → syscall_handler.
global syscall_stub
syscall_stub:
    cli
    push 0              ; dummy error code
    push 0x80           ; interrupt number
    jmp isr_common

; --- Common IRQ handler ---
irq_common:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    extern irq_handler
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret
