[bits 32]
global switch_to
switch_to:
    ; 保存上下文 所谓ABI
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20] ; cur
    mov [eax], esp  ; cur->kstack   保存栈顶


    mov eax, [esp + 24]
    mov esp, [eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
    