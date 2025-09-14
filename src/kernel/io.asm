[bits 32]

global inb
inb:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ;端口号
    in al, dx
    leave
    ret

global outb
outb:
    push ebp
    mov ebp, esp
    ;端口号
    mov edx, [ebp + 8]
    ;值
    mov eax, [ebp + 12]
    out dx, al

    leave
    ret

global inw
inw:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ;端口号
    xor ax, ax
    in al, dx
    shl ax, 4
    in al, dx
    leave
    ret
    
global outw
outw:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    mov eax, [ebp + 12]
    out dx, al
    shr ax, 4
    out dx, al

    leave
    ret