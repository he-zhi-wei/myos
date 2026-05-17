section .multiboot
align 4
    dd 0x1BADB002          ; multiboot magic
    dd 0x03                ; flags: page-align + memory info
    dd -(0x1BADB002 + 0x03) ; checksum

section .text
global _start
_start:
    mov esp, stack_top     ; set up 16KB stack
    push eax               ; push multiboot magic (2nd param)
    push ebx               ; push multiboot info ptr (1st param)
    extern kernel_main
    call kernel_main
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384             ; 16KB stack
stack_top:
