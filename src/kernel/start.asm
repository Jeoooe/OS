[bits 32]

extern console_init
extern memory_init
extern kernel_main
extern tss_init
global _start
_start:
    push ebx    ; ards_count
    push eax    ; gdt_base
    call console_init
    call memory_init
    call tss_init
    call kernel_main
    jmp $