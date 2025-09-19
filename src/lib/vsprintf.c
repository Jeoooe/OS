#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


#define ZEROPAD     1   //0填充
#define SIGN        2   
#define PLUS        4   //+
#define SPACE       8   //空格
#define LEFT        16  //左对齐
#define SPECIAL     32  //0x
#define SMALL       64  //小写

#define is_digit(c) ((c) >= '0' && (c) <= '9')

//转整数
static int skip_atoi(const char **s) {
    int i = 0;
    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

static char *number(char *str, unsigned long num, int base, int size, int precision, int flags) {
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char tmp[1024];
    char c, sign = 0;
    int i;
    int index;

    if (flags & SMALL) {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }

    if (flags & LEFT) {
        flags &= ~ZEROPAD;
    }

    if (base < 2 || base > 36) {
        return 0;
    }

    //填充符
    c = (flags & ZEROPAD) ? '0' : ' ';
    
    if (num < 0 && flags & SIGN) {
        sign = '-';
        num = -num;
    } else {
        sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
    }
    if (sign) size--;
    
    if (flags & SPECIAL) {
        if (base == 16) size -= 2;
        else if (base == 8) size --;
    }

    i = 0;
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            index = num % base;
            num /= base;
            tmp[i++] = digits[index];
        }
    }

    if (i > precision) {
        precision = i;
    }
    size -= precision;

    if (!(flags & (ZEROPAD + LEFT))) {
        for (;size > 0; --size) {
            *str = ' ';
            ++str;
        }
    }

    if (sign) {
        *str = sign;
        ++str;
    }

    if (flags & SPECIAL) {
        if (base == 8) {
            *str = '0';
            ++str;
        } else if (base == 16) {
            *str++ = '0';
            *str++ = 'x';
        }
    }

    if (! (flags & LEFT)) {
        for (;size > 0;--size) {
            *str = c;
            ++str;
        }
    }
    
    for (;i < precision; --precision) {
        *str = '0';
        ++str;
    }

    --i;
    for (;i>=0; --i) {
        *str = tmp[i];
        ++str;
    }

    for (;size > 0;--size) {
        *str = ' ';
        ++str;
    }
    return str;
}

int vsprintf(char *buf, const char *fmt, va_list args) {
    int flags, field_width, precision, len, i;
    char qualifier = 0;
    char *str = NULL;
    char *s = NULL;
    int *ip;

    for (str = buf; *fmt; ++fmt) {
        if (*fmt != '%') {
            *str = *fmt;
            ++str;
            continue;
        }

        flags = 0;
    repeat:
        fmt++;
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        //宽度
        field_width = -1;

        if (is_digit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            ++fmt;
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        //精度
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt)) {
                precision = skip_atoi(&fmt);
            } else if (*fmt == '*') {
                ++fmt;
                precision = va_arg(args, int);
            }

            if (precision < 0) precision = -precision;
        }

        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        switch (*fmt) {
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
        case 's':
            s = va_arg(args, char *);
            len = strlen(s);
            if (precision < 0) precision = len;
            else if (len > precision) len=precision;

            if (!(flags & LEFT)) {
                for (;len<field_width;--field_width) {
                    *str++ = ' ';
                }
            }
            for (i = 0;i < len; ++i) {
                *str++ = *s++;
            }

            for (;len < field_width; --field_width) {
                *str++ = ' ';
            }
            break;

        case 'o':
            str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flags);
            break;
        case 'p':
            if (field_width == -1) {
                field_width = 8;
                flags |= ZEROPAD;
            }
            str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
            break;
        case 'x':
            flags |= SMALL;
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
            break;
        
        case 'n':
            ip = va_arg(args, int *);
            *ip = str - buf;
            break;
        default:
            if (*fmt != '%') {
                *str++ = '%';
            }
            if (*fmt) {
                *str++ = *fmt;
            } else {
                --fmt;
            }
            break;
        }
    }
    *str = '\0';

    i = str - buf;
    assert(i < 1024);
    return str - buf;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(buf, fmt, args);
    va_end(args);
    return i;
}