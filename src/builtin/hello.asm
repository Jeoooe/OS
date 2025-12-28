segment .text
global _start
_start:
    mov eax, 4
    mov ebx, 1
    mov ecx, hello
    mov edx, 7
    int 0x80

    ; exit
    mov eax, 1
    mov ebx, 0
    int 0x80


segment .data
hello:
    dd "Hello", 10, 0