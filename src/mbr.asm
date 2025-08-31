
section mbr vstart=0x7c00
    mov ax,cs 
    mov ds,ax 
    mov es,ax 
    mov ss,ax 
    mov fs,ax 
    mov sp,0x7c00 

    xchg bx, bx

    ;清屏
    mov ah, 0x06
    mov al, 0
    mov bx, 0x700
    mov cx, 0
    mov dl, 0x4f
    mov dh, 0x18
    int 0x10

    mov ah, 3
    mov bh, 0
    int 0x10

    mov ax, message
    mov bp, ax
    mov cx, 5
    mov ah, 0x13
    mov al, 0x01
    mov bx, 0x2
    int 0x10

    jmp $

    message:
        db "MBR"
    times 510-($-$$) db 0
    db 0x55, 0xaa