[bits 32]

extern console_init
extern memory_init
extern kernel_main
global _start
_start:
    call console_init
    push ebx    ; ards_count
    push eax    ; magic
    call memory_init
    call kernel_main
    jmp $