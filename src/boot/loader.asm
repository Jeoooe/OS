section loader vstart=0x900
    mov ax, 0xb800
    mov gs, ax

    mov byte [gs:0x00], '2'
    mov byte [gs:0x01], 0xa4
    mov byte [gs:0x02], 'l'
    mov byte [gs:0x03], 0xa4
    mov byte [gs:0x04], 'o'
    mov byte [gs:0x05], 0xa4
    mov byte [gs:0x06], 'a'
    mov byte [gs:0x07], 0xa4
    mov byte [gs:0x08], 'd'
    mov byte [gs:0x09], 0xa4
    mov byte [gs:0x0a], 'e'
    mov byte [gs:0x0b], 0xa4
    mov byte [gs:0x0c], 'r'
    mov byte [gs:0x0d], 0xa4

    jmp $