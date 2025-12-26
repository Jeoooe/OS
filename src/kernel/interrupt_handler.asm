[bits 32]

; extern tmp_handler
extern handler_table    ;中断处理函数的数组,在interrupt.c中定义

%define ERROR nop
%define ZERO push 0
%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
    %2
    ;保存上下文
    push ds
    push es
    push fs
    push gs
    pushad  ;压入32位寄存器

    ;发送处理完成信号
    mov al, 0x20
    out 0xa0, al
    out 0x20, al

    push %1 ;压入中断向量号
    call [handler_table + %1 * 4]
    jmp intr_exit
%endmacro

global intr_exit
intr_exit:
    add esp, 4  ;跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4
    iret


INTERRUPT_HANDLER 0x00, ZERO
INTERRUPT_HANDLER 0x01, ZERO
INTERRUPT_HANDLER 0x02, ZERO
INTERRUPT_HANDLER 0x03, ZERO
INTERRUPT_HANDLER 0x04, ZERO
INTERRUPT_HANDLER 0x05, ZERO
INTERRUPT_HANDLER 0x06, ZERO
INTERRUPT_HANDLER 0x07, ZERO
INTERRUPT_HANDLER 0x08, ERROR
INTERRUPT_HANDLER 0x09, ZERO
INTERRUPT_HANDLER 0x0a, ERROR
INTERRUPT_HANDLER 0x0b, ERROR
INTERRUPT_HANDLER 0x0c, ERROR
INTERRUPT_HANDLER 0x0d, ERROR
INTERRUPT_HANDLER 0x0e, ERROR
INTERRUPT_HANDLER 0x0f, ZERO
INTERRUPT_HANDLER 0x10, ZERO
INTERRUPT_HANDLER 0x11, ERROR
INTERRUPT_HANDLER 0x12, ZERO
INTERRUPT_HANDLER 0x13, ZERO
INTERRUPT_HANDLER 0x14, ZERO
INTERRUPT_HANDLER 0x15, ZERO
INTERRUPT_HANDLER 0x16, ZERO
INTERRUPT_HANDLER 0x17, ZERO
INTERRUPT_HANDLER 0x18, ZERO
INTERRUPT_HANDLER 0x19, ZERO
INTERRUPT_HANDLER 0x1a, ZERO
INTERRUPT_HANDLER 0x1b, ZERO
INTERRUPT_HANDLER 0x1c, ZERO
INTERRUPT_HANDLER 0x1d, ZERO
INTERRUPT_HANDLER 0x1e, ZERO
INTERRUPT_HANDLER 0x1f, ZERO
INTERRUPT_HANDLER 0x20, ZERO    ;从这里开始是中断
INTERRUPT_HANDLER 0x21, ZERO    ;键盘中断 
INTERRUPT_HANDLER 0x22, ZERO    
INTERRUPT_HANDLER 0x23, ZERO
INTERRUPT_HANDLER 0x24, ZERO
INTERRUPT_HANDLER 0x25, ZERO
INTERRUPT_HANDLER 0x26, ZERO
INTERRUPT_HANDLER 0x27, ZERO
INTERRUPT_HANDLER 0x28, ZERO
INTERRUPT_HANDLER 0x29, ZERO
INTERRUPT_HANDLER 0x2a, ZERO
INTERRUPT_HANDLER 0x2b, ZERO
INTERRUPT_HANDLER 0x2c, ZERO
INTERRUPT_HANDLER 0x2d, ZERO
INTERRUPT_HANDLER 0x2e, ZERO
INTERRUPT_HANDLER 0x2f, ZERO

;interrupt_handler_%1:
global handler_entry_table
handler_entry_table:
    dd interrupt_handler_0x00 
    dd interrupt_handler_0x01 
    dd interrupt_handler_0x02 
    dd interrupt_handler_0x03 
    dd interrupt_handler_0x04 
    dd interrupt_handler_0x05 
    dd interrupt_handler_0x06 
    dd interrupt_handler_0x07 
    dd interrupt_handler_0x08 
    dd interrupt_handler_0x09 
    dd interrupt_handler_0x0a 
    dd interrupt_handler_0x0b 
    dd interrupt_handler_0x0c 
    dd interrupt_handler_0x0d 
    dd interrupt_handler_0x0e 
    dd interrupt_handler_0x0f 
    dd interrupt_handler_0x10 
    dd interrupt_handler_0x11 
    dd interrupt_handler_0x12 
    dd interrupt_handler_0x13 
    dd interrupt_handler_0x14 
    dd interrupt_handler_0x15 
    dd interrupt_handler_0x16 
    dd interrupt_handler_0x17 
    dd interrupt_handler_0x18 
    dd interrupt_handler_0x19 
    dd interrupt_handler_0x1a 
    dd interrupt_handler_0x1b 
    dd interrupt_handler_0x1c 
    dd interrupt_handler_0x1d 
    dd interrupt_handler_0x1e 
    dd interrupt_handler_0x1f 
    dd interrupt_handler_0x20     ;从这里开始是中断
    dd interrupt_handler_0x21 
    dd interrupt_handler_0x22 
    dd interrupt_handler_0x23 
    dd interrupt_handler_0x24 
    dd interrupt_handler_0x25 
    dd interrupt_handler_0x26 
    dd interrupt_handler_0x27 
    dd interrupt_handler_0x28 
    dd interrupt_handler_0x29 
    dd interrupt_handler_0x2a 
    dd interrupt_handler_0x2b 
    dd interrupt_handler_0x2c 
    dd interrupt_handler_0x2d 
    dd interrupt_handler_0x2e 
    dd interrupt_handler_0x2f 



;系统调用
extern syscall_table
extern do_signal
global syscall_handler
syscall_handler:
    push 0
    push ds
    push es
    push fs
    push gs
    pushad

    push 0x80 ;中断向量号

    push edx
    push ecx
    push ebx

    call [syscall_table + eax * 4]
    add esp, 12
    
    mov [esp + 32], eax ;中断栈中eax的位置，作为返回值
    ;然后是调用信号处理函数
    call do_signal

    jmp intr_exit

global sa_restorer
sa_restorer:
    ;   信号处理恢复函数
    add esp, 8  ;跳过signr 和 blocked
    pop eax
    pop ecx
    pop edx
    popf
    ret