[bits 32]

global get_interrupt_state
global interrupt_disable
global interrupt_enable

get_interrupt_state:
    pushf
    pop eax
    shr eax, 9
    and eax, 1
    ret

interrupt_disable:
    pushf
    pop eax
    cli
    shr eax, 9
    and eax, 1
    ret

interrupt_enable:
    pushf
    pop eax
    sti
    shr eax, 9
    and eax, 1
    ret