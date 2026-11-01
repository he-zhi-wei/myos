; 文件：embed.asm
; 作用：用 incbin 把用户程序 init.elf 原样嵌入内核镜像，导出起止符号供 C 侧 ELF 加载器读取。
; 要点：数据位于 .rodata（只读，运行时不可改写）；user_bin_start/user_bin_end 直接以 C 数组符号引用，长度 = end - start；incbin 路径相对 nasm 的 -I 包含目录，构建前 init.elf 必须已生成。
; Embed user.bin as raw binary data. Symbols are directly callable from C.
section .rodata
global user_bin_start
global user_bin_end
user_bin_start:
    incbin "init.elf"
user_bin_end:
