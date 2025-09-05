[org 0x7c00]

;文本模式
mov ax, 3
int 0x10

;段寄存器初始化
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

;;;;;中断调试

;打印字符串
; xchg bx, bx;bochs
mov si, booting
call print

;硬盘读取
mov edi, 0x1000     ;读取到目标内存 
mov ecx, 2          ;起始扇区
mov bl, 4           ;扇区数量 
call read_disk

;加载内核
cmp word [0x1000], 0x55aa
jnz error
jmp 0:0x1002

; 阻塞
jmp $

read_disk:
    mov dx, 0x1f2
    mov al, bl
    out dx, al

    inc dx ;0x1f3
    mov al, cl; 起始扇区前八位
    out dx, al

    inc dx ;0x1f4
    shr ecx, 8 ; >>8
    mov al, cl; 起始扇区中八位
    out dx, al

    inc dx ;0x1f5
    shr ecx, 8 ; >>8
    mov al, cl; 起始扇区高八位
    out dx, al

    inc dx; 0x1f6
    shr ecx, 8
    and cl, 0b1111;
    mov al, 0b1110_0000;
    or al, cl
    out dx, al  ;主盘 - LBA模式

    inc dx; 0x1f7
    mov al,0x20 ;读硬盘
    out dx, al

    ;获取数据
    xor ecx, ecx
    mov cl, bl  ;读写扇区数量

    .read:
        push cx ;保存cx
        call .waits ;等待数据准备
        call .reads ;读取扇区
        pop cx  ;恢复cx
        loop .read
    ret

    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx
            jmp $+2;
            jmp $+2
            jmp $+2
            and al, 0b1000_1000
            cmp al, 0b0000_1000
            jnz .check
        ret

    .reads:
        mov dx, 0x1f0
        mov cx, 256 ;扇区大小
        .readw:
            in ax, dx
            jmp $+2
            jmp $+2
            jmp $+2
            mov [edi], ax
            add edi, 2
            loop .readw
        ret


;阻塞之后的内容仅可作为数据和函数调用
print:
    mov ah, 0x0e
.next:
    mov al, [si]
    cmp al, 0
    jz .done
    int 0x10
    inc si
    jmp .next
.done :
    ret


booting:
    db "Booting os...", 13, 10, 0; \n \r \0

error:
    mov si, .msg
    call print
    hlt ;cpu停止
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0


;填充为0
times 510 - ($ - $$) db 0

;主引导扇区最后2字节要求
db 0x55, 0xaa