#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define SPECIAL 1
#define ZERO 2
#define LEFT 4
#define SPACE 8
#define PLUS 16
#define SMALL 32
#define SIGN 64

#define is_digit(ch) ((ch) >= '0' && (ch) <= '9')

//字符串转数字,并将指针指向下一个字符
static int str_to_number(const char **ptr) {
    int value = 0;
    while (is_digit(**ptr)) {
        value = value * 10 + (**ptr - '0');
        (*ptr)++;
    }
    return value;
}

static char* number(char* str, int num, int base, int size, int precision, int type) {
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char tmp[36];
    char c, sign;
    int i;
    if (type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (type & LEFT) type &= ~ZERO;
    //只能处理2进制到36进制
    if (base < 2 || base > 36)
        return 0;
    c = (type & ZERO) ? '0' : ' ';
    if (type & SIGN && num < 0) {
        sign = '-';
        num = -num;
    }
    else {
        sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
    }
    //有符号位
    if (sign) size--;
    //特殊转换
    if (type & SPECIAL) {
        if (base == 16) size -= 2;
        else if (base == 8) size --;
    }
    i = 0;
    if (num == 0) tmp[i++] = '0';
    else {
        while (num != 0) {
            tmp[i++] = digits[num % base];
            num /= base;
        }   
    }
    if (i > precision) precision = i;
    size -= precision;

    //生成转换结果
    if (!(type & (ZERO | LEFT))) {
        while (size-- > 0) *str++ = ' ';
    }
    if (sign) *str++ = sign;
    if (type & SPECIAL) {
        if (base == 8) *str++ = '0';
        else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(type & LEFT)) {
        while (size-- > 0) {
            *str++ = c;
        }
    }

    while (i < precision--) {
        *str++ = '0';
    }
    while (i-- > 0) {
        *str++ = tmp[i];
    }
    //宽度大于0,有左对齐
    while (size-- > 0) {
        *str++ = ' ';
    }
    return str;
}

int vsprintf(char* buf, const char* fmt, va_list args) {
    char *str = buf;
    char *s = NULL;
    int flags = 0;
    int field_width = 0;
    int precision = -1;
    int len = 0;
    int i = 0;
    int *ip = NULL;
    for(;*fmt != EOS;fmt++) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }
        flags = 0;
        repeat:
        fmt++;
        //flags
        switch (*fmt) {
            case '#': flags |= SPECIAL; goto repeat;
            case '0': flags |= ZERO;    goto repeat;
            case ' ': flags |= SPACE;   goto repeat;
            case '-': flags |= LEFT;    goto repeat;
            case '+': flags |= PLUS;    goto repeat;
        }
        //width
        field_width = -1;
        if (is_digit(*fmt)) {
            field_width = str_to_number(&fmt);
        }
        else if (*fmt == '*') {
            fmt++;
            field_width = va_arg(args, int);
        }
        //precision
        precision = -1;
        if (*fmt == '.') {
            fmt++;
            if (is_digit(*fmt)) {
                precision = str_to_number(&fmt);
            } 
            else if (*fmt == '*') {
                fmt++;
                precision = va_arg(args, int);
            }
            if (precision < 0) precision = 0;
        }

        //输出
        switch (*fmt) {
        //单个字符
        case 'c':
            if (!(flags & LEFT)) {
                while (--field_width > 0) {
                    *str++ = ' ';
                }
            }
            *str++ = (unsigned char) va_arg(args, int);
            while (--field_width > 0) {
                    *str++ = ' ';
            }
            break;
        //字符串
        case 's':
            //获取长度
            s = va_arg(args, char *);
            len = strlen(s);
            if (precision < 0) precision = len;
            else if (precision < len) len = precision;
            if (!(flags & LEFT)) {
                while (field_width-- > len) {
                    *str++ = ' ';
                }
            }
            for (i = 0;i < len;i++) {
                *str++ = *s++;
            }
            while (field_width-- > len) {
                    *str++ = ' ';
            }
            break;
        //八进制
        case 'o':
            str = number(str, va_arg(args, unsigned long), 8,
             field_width, precision, flags);
             break;
        //指针
        case 'p':
            if (field_width == -1) {
                field_width = 8;
                flags |= ZERO;
            }
            str = number(str, (unsigned long)va_arg(args, void *), 16,
             field_width, precision, flags);
             break;
        //十六进制
        case 'x':
            flags |= SMALL;
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16,
             field_width, precision, flags);
            break;
        //整数
        //d,i 符号整数
        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10,
             field_width, precision, flags);
             break;
        //保存指针
        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;
        default:
            if (*fmt != '%') *str++ = '%';
            if (*fmt) *str++ = *fmt;
            else --fmt;
        }
    }
    *str = '\0';
    return str - buf;
}