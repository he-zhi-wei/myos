; Embed user.bin as raw binary data. Symbols are directly callable from C.
section .rodata
global user_bin_start
global user_bin_end
user_bin_start:
    incbin "user.elf"
user_bin_end:
