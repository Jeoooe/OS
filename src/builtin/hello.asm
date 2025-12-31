segment .text
global _start
_start:
    mov eax, 4
    mov ebx, 1
    mov ecx, hello
    mov edx, 14
    int 0x80

    ; exit
    mov eax, 1
    mov ebx, 65
    int 0x80
hello:
    db "Hello World!", 10, 0

