; 文件：boot.asm
; 作用：Multiboot 内核入口，建立 16KB 栈后携带 magic 与 multiboot 信息指针跳入 C 的 kernel_main。
; 要点：.multiboot 段头为 magic 0x1BADB002、flags 0x03（页对齐 + 内存信息）及其校验和，供 GRUB 识别；EAX(magic) 为第 1 参数、EBX(mb_info) 为第 2 参数；kernel_main 返回后关中断并停机。
section .multiboot
align 4
    dd 0x1BADB002          ; multiboot magic
    dd 0x03                ; flags: page-align + memory info
    dd -(0x1BADB002 + 0x03) ; checksum

section .text
global _start
_start:
    mov esp, stack_top     ; set up 16KB stack
    push ebx               ; multiboot info ptr → addr (2nd param)
    push eax               ; multiboot magic → magic (1st param)
    extern kernel_main
    call kernel_main
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384             ; 16KB stack
stack_top:
