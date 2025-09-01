;宏
LOADER_START_LBA equ 0x2        ;loader起始扇区
LOADER_TARGET_MEMORY equ 0x900  ;loader运行目标内存地址
LOADER_SECOTRS equ 1

section mbr vstart=0x7c00
    mov ax,cs 
    mov ds,ax 
    mov es,ax 
    mov ss,ax 
    mov fs,ax 
    mov sp,0x7c00 

    ;清屏
    mov ah, 0x06
    mov al, 0
    mov bx, 0x700
    mov cx, 0
    mov dl, 0x4f
    mov dh, 0x18
    int 0x10

    mov ax, 0xb800
    mov gs, ax
    mov byte [gs:0x00], 'M'
    mov byte [gs:0x01], 0xa4
    mov byte [gs:0x02], 'B'
    mov byte [gs:0x03], 0xa4
    mov byte [gs:0x04], 'R'
    mov byte [gs:0x05], 0xa4

    ;读取loader
    mov eax, LOADER_START_LBA
    mov bx, LOADER_TARGET_MEMORY
    mov cx, LOADER_SECOTRS
    call read_disk_16

    jmp LOADER_TARGET_MEMORY

    ;程序错误
    jmp $

;读取硬盘
;eax 起始扇区
;bx 目标地址
;cx 扇区数
read_disk_16:
    ;选择主通道
    ;写入扇区数
    push ax
    mov ax, cx
    mov dx, 0x1f2
    out dx, ax
    pop ax
    ;写入lba低24位
    mov dx, 0x1f3
    out dx, al
    mov dx, 0x1f4
    shr ax, 8
    out dx, al
    mov dx, 0x1f5
    shr ax, 8
    out dx, al
    ;lba 24-27位
    mov dx, 0x1f6
    and al, 0xff
    or al, 0b11100000
    out dx, al
    ;command
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ;判断status
    .status:
        in al, dx
        and al, 0b10001000
        cmp al, 0b00001000
        jmp $+2
        jmp $+2
        jmp $+2
        jnz .status
    
    
    ;读取数据
    ;读取扇区数*512字节/2每次一个字
    mov ax, 256
    mul cx
    mov cx, ax
    mov dx, 0x1f0
    .read:
        ;读取一个字
        jmp $+2
        jmp $+2
        jmp $+2
        in ax, dx
        mov [bx], ax
        add bx, 2
        loop .read
    
    ret
    

message:
    db "MBR"
    
times 510-($-$$) db 0
db 0x55, 0xaa