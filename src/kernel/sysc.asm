; 目前只有fork一个函数


global fork
fork:
    mov eax, 57
    int 0x80 
    ret