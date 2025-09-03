;loader
;进入保护模式 

;宏
LOADER_BASE_ADDR equ 0x900  ;loader运行目标内存地址


section loader vstart=LOADER_BASE_ADDR
    [bits 16]
    mov ax, LOADER_BASE_ADDR
    mov sp, ax

    ;调用bios内存检测
    ;内存检测
    ;子功能号 0xe820
    xor ebx, ebx
    mov di, ards_start
    mov ecx, 20
    mov edx, 0x534d4150 ;固定签名
    .read_ards:
        mov eax, 0xe820
        int 0x15
        jc .error   ;cf=1,出错
        add di, cx
        cmp ebx, 0
        jnz .read_ards


    ;全局描述符表
    ;加载gdt
    lgdt [gdt_ptr]
    ;打开a20
    in al, 0x92
    or al, 0b10
    out 0x92, al
    ;pe置为1
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    ;跳转
    jmp dword gdt_code_selector:protection_mode

.error:
    jmp $



    [bits 32]
protection_mode:
    
    mov eax, gdt_data_selector
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    mov esp, LOADER_BASE_ADDR

    ;加载内核文件
    mov eax, KERNEL_BIN_SECTOR_START
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, KERNEL_BIN_SECTOR_COUNT
    call read_disk_32
    

    ;开启分页机制
    call setup_page
    ;赋值cr3,并置cr0的PG位
    mov eax, PAGE_DIR_BASE
    mov cr3, eax
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax

    ;初始化内核
    ;将内核程序映射
    call kernel_init
    xchg bx, bx
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY
    jmp $

kernel_init:
    xor eax, eax
    xor edx, edx
    xor ebx, ebx
    xor ecx, ecx
    ;程序表项大小
    mov dx, [KERNEL_BIN_BASE_ADDR + 42]
    ;程序表头基址
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]
    add ebx, KERNEL_BIN_BASE_ADDR
    ;程序表项数目
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]
    .each_segment:
        cmp byte [ebx], PT_TYPE_NULL
        je .jumpto_next_segment

        ;复制段
        ;size
        push dword [ebx + 16]
        ;src
        mov eax, [ebx + 4]
        add eax, KERNEL_BIN_BASE_ADDR
        push eax
        ;dest
        push dword [ebx + 8]
        call mem_cpy
        add esp, 12

        .jumpto_next_segment:
        add ebx, edx
        loop .each_segment
    ret

;复制字节
;dest, src, size
mem_cpy:
    cld

    ; xchg bx, bx

    push ebp
    mov ebp, esp
    push ecx
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
    rep movsb

    ; xchg bx, bx

    pop ecx
    pop ebp

    ret

;读取硬盘
;eax 起始扇区
;ebx 目标地址
;cx 扇区数
read_disk_32:
    ;选择主通道
    ;写入扇区数
    push eax
    mov ax, cx
    mov dx, 0x1f2
    out dx, ax
    pop eax
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
        mov [ebx], ax
        add bx, 2
        loop .read
    
    ret

;分配页表
setup_page:
    ;先把页目录置空
    mov esi, 0
    mov ecx, 0x1000
    .clear_page_dir:
        mov byte [PAGE_DIR_BASE + esi], 0
        inc esi
        loop .clear_page_dir
    ;设置最后一个页目录项为页目录地址
    mov eax, PAGE_DIR_BASE
    or eax, PAGE_RW_W | PAGE_US_U | PAGE_PRESENT
    mov [PAGE_DIR_BASE + 4092], eax
    ;设置0和768页目录项为第0个页表0x11000
    ;即0xC000_0000物理内存, 3GB起点
    add eax, 0x1000
    mov [PAGE_DIR_BASE], eax
    mov [PAGE_DIR_BASE + 768 * 4], eax
    ;初始化第0页表
    ;内核内存仅1M,则映射256页
    mov eax, PAGE_DIR_BASE  ;eax页目录物理地址
    add eax, 0x1000
    mov ecx, 256
    mov esi, 0
    mov edx, PAGE_PRESENT | PAGE_RW_W | PAGE_US_U   ;物理地址0开始
    .set_kernel_memory_page:
        mov [eax + esi * 4], edx
        inc esi
        add edx, 0x1000
        loop .set_kernel_memory_page
    ;设置769到1022页目录项
    mov eax, PAGE_DIR_BASE  ;eax页目录物理地址
    mov ecx, 254
    mov edx, PAGE_DIR_BASE + 0x2000
    or edx, PAGE_PRESENT | PAGE_RW_W | PAGE_US_U
    mov esi, 769
    .set_high_1g:
        mov [eax + esi * 4], edx
        inc esi
        add edx, 0x1000
        loop .set_high_1g
    ret
    

;内核部分
KERNEL_BIN_BASE_ADDR equ 0x70000
KERNEL_BIN_SECTOR_START equ 0x9
KERNEL_BIN_SECTOR_COUNT equ 10
PT_TYPE_NULL equ 0
KERNEL_ENTRY equ 0x1500

; 页表部分
; 页目录物理地址
PAGE_DIR_BASE equ 0x100000
PAGE_PRESENT equ 1
PAGE_RW_R equ (0 << 1)
PAGE_RW_W equ (1 << 1)
PAGE_US_S equ (0 << 2)  ;;超级用户 特权级0
PAGE_US_U equ (1 << 2)  ;;普通用户 特权级3

;GDT部分
;选择子
gdt_code_selector equ (1 << 3)
gdt_data_selector equ (2 << 3)

;先低位再高位
;小端字节序
BASE_HIGH8 equ 0x00 << 24
BASE_LOW8 equ 0
GDT_S_SYSTEM equ 0 << 12
GDT_S_DATA equ 1 << 12
GDT_TYPE_CODE equ 0b1000 << 8   ;xcra
GDT_TYPE_DATA equ 0b0010 << 8   ;xewa
GDT_DPL equ 0b00 << 13
GDT_PRESENT equ 0b1<<15
GDT_LIMIT_HIGH4 equ 0xf<<16
GDT_AVL equ 1 << 20
GDT_L equ 0 << 21
GDT_D equ 1 << 22
GDT_G equ 1 << 23

;代码段描述符
GDT_CODE_DESC_HIGH32 equ BASE_LOW8 | GDT_S_DATA | GDT_TYPE_CODE | \
    GDT_DPL | GDT_PRESENT | GDT_LIMIT_HIGH4 | GDT_AVL | GDT_L | \
    GDT_D | GDT_G
;数据段段描述符
GDT_DATA_DESC_HIGH32 equ BASE_LOW8 | GDT_S_DATA | GDT_TYPE_DATA | \
    GDT_DPL | GDT_PRESENT | GDT_LIMIT_HIGH4 | GDT_AVL | GDT_L | \
    GDT_D | GDT_G

;gdt表界限
gdt_limit equ gdt_end - gdt_start - 1

gdt_ptr:
    dw gdt_limit
    dd gdt_start

gdt_start:
    dd 0, 0
gdt_code:
    dd 0x0000ffff
    dd GDT_CODE_DESC_HIGH32
gdt_data:
    dd 0x0000ffff
    dd GDT_DATA_DESC_HIGH32
gdt_end:
    nop
ards_start: