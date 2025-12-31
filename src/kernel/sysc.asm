; 目前只有fork一个函数


global fork
fork:
    mov eax, 2
    int 0x80 
    ret