[bits 32]

global inb
inb:
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