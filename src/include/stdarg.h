#ifndef OS_STDARG_H
#define OS_STDARG_H

typedef char* va_list;

//字节对齐
//小于4按4字节对齐,大于4则用4的最小倍数
#define __va_rounded_size(type) \
(((sizeof(type) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

//可变参数起始
#define va_start(ap, last) \
    ap = ((va_list)&(last) + __va_rounded_size(last))
//取下一个参数
#define va_arg(ap, type) \
    (ap += __va_rounded_size(type), *((type *)(ap - __va_rounded_size(type))))

#define va_end(ap) (ap = (va_list)0)

#endif