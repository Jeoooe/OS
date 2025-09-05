[org 0x1000]

dw 0x55aa; 魔数 判断错误



mov si, loading
call print

detect_memory:
    xor ebx, ebx

    ; es:di 缓存位置
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer

    mov edx, 0x534d4150 ;签名

.next:
    mov eax, 0xe820 ;子功能号
    ;结构体大小
    mov ecx, 20

    int 0x15

    ;CF = 1
    jc error

    add di, cx

    ;结构体数量+1
    inc dword [ards_count]

    cmp ebx, 0
    jnz .next

    mov si, detecting
    call print

    jmp prepare_protected_mode

;准备保护模式
prepare_protected_mode:

    cli ;关闭中断
    ;打开A20线
    in  al, 0x92
    or al, 0b10
    out 0x92, al

    ;加载gdt全局描述符
    lgdt [gdt_ptr]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword code_selector:protect_mode

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


loading:
    db "Loading os...", 13, 10, 0; \n \r \0
detecting:
    db "Detecting Memory Success...", 13, 10, 0; \n \r \0

error:
    mov si, .msg
    call print
    hlt ;cpu停止
    jmp $
    .msg db "Loading Error!!!", 10, 13, 0


;进入了保护模式
[bits 32]
protect_mode:
    ;初始化段寄存器
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x10000;栈顶

    ;读取内核
    mov edi, 0x10000     ;把内核读取到内存0x10000 
    mov ecx, 10          ;内核起始扇区
    mov bl, 200           ;扇区数量 
    call read_disk

    mov eax, 0x20250307
    mov ebx, ards_count

    jmp dword code_selector:0x10000

    ud2         ;表示出错

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

code_selector equ (1<<3)
data_selector equ (2<<3)

memory_base equ 0
memory_limit equ 1024*1024 - 1 ; 4GB - 1 (4kb粒度)

;gdt指针
gdt_ptr:
    dw (gdt_end - gdt_base) - 1
    dd gdt_base
gdt_base:
    dd 0, 0;    NULL 描述符
;代码段
gdt_code:
    dw memory_limit & 0xffff
    dw memory_base & 0xffff 
    db (memory_base >> 16) & 0xff
    db 0b_1_00_1_1_0_1_0    
    db 0b_1_1_0_0_0000 | ((memory_limit >> 16) & 0xf)
    db (memory_base >> 24) & 0xff
;数据段
gdt_data:
    dw memory_limit & 0xffff
    dw memory_base & 0xffff 
    db (memory_base >> 16) & 0xff
    db 0b_1_00_1_0_0_1_0    
    db 0b_1_1_0_0_0000 | ((memory_limit >> 16) & 0xf)
    db (memory_base >> 24) & 0xff
gdt_end:

ards_count:
    dd 0
ards_buffer:    ;BIOS 内存检测缓存
    