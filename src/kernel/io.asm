[bits 32]

global inb
inb:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ;端口号
    in al, dx
    leave
    ret

global outb
outb:
    push ebp
    mov ebp, esp
    ;端口号
    mov edx, [ebp + 8]
    ;值
    mov eax, [ebp + 12]
    out dx, al

    leave
    ret

global inw
inw:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ;端口号
    xor ax, ax
    in al, dx
    shl ax, 4
    in al, dx
    leave
    ret
    
global outw
outw:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    mov eax, [ebp + 12]
    out dx, al
    shr ax, 4
    out dx, al

    leave
    ret

; global _insw
; _insw:
;     push ebp
;     mov ebp, esp
;     push edi

;     mov dx, [ebp + 8]
;     mov di, [ebp + 12]
;     mov cx, [ebp + 16]

;     cld
;     rep insw

;     pop edi
;     leave
;     ret

; global _outsw
; _outsw:
;     push ebp
;     mov ebp, esp
;     push esi

;     mov dx, [ebp + 8]
;     mov si, [ebp + 12]
;     mov cx, [ebp + 16]

;     cld
;     rep outsw

;     pop esi
;     leave
;     ret