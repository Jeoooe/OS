[bits 32]

extern tmp_handler

%define ERROR nop
%define ZERO push 0

%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
    %2
    call tmp_handler
    mov al, 0x20
    out 0xa0, al
    out 0x20, al
    add esp, 4
    iret
%endmacro



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