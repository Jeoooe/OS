#include <string.h>
#include <stdint.h>

char *strcpy(char *dest, const char *src) {
    char *ptr = dest;
    while (true) {
        *ptr = *src;
        ++ptr;
        if (*src == EOS) 
            return dest;
        ++src;
    }
}
char *strncpy(char *dest, const char *src, size_t count) {
    char *ptr = dest;
    size_t nr = 0;
    for (;nr < count;nr++) {
        *ptr++ = *src;
        if (*src++ == EOS) {
            return dest;
        }
    }
    dest[count - 1] = EOS;
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *ptr = dest;
    while (*ptr != EOS) {
        ptr++;
    }
    while (true) {
        *ptr = *src;
        ++ptr; 
        if (*src == EOS) 
            return dest;
        src++;
    }
}
size_t strlen(const char *str) {
    char *ptr = (char *) str;
    while (*ptr != EOS) ++ptr;
    return ptr - str;
}
int strcmp(const char *lhs, const char *rhs) {
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS) {
        ++lhs;
        ++rhs;
    }
    return *lhs < *rhs ? -1 : *lhs > *rhs;
}
char *strchr(const char *str, int ch) {
    char *ptr = (char *)str;
    while (true) {
        if (*ptr == ch) {
            return ptr;
        }
        if (*ptr == EOS) {
            return NULL;
        }
        ++ptr;
    }
}
char *strrchr(const char *str, int ch) {
    char *last = NULL;
    char *ptr = (char *)str;
    while (true) {
        if (*ptr == ch) last = ptr;
        if (*ptr == EOS) return last;
        ++ptr;
    }
}

int memcmp(const void *lhs, const void *rhs, size_t count) {
    char *lptr = (char *)lhs;
    char *rptr = (char *)rhs;
    while (*lptr == *rptr && count > 0) {
        ++lptr;
        ++rptr;
        --count; 
    }
    return *lptr < *rptr ? -1 : *lptr > *rptr;
};
void *memset(void *dest, int ch, size_t count) {
    char *ptr = dest;
    while (count) {
        *ptr = ch;
        ++ptr;
        --count;
    }
    return dest;
}
//复制内存数据
void *memcpy(void *dest, const void *src, size_t count) {
    char *ptr = dest;
    while (count) {
        *ptr = *((char *)src);
        ++src;
        ++ptr;
        --count;
    }
    return dest;
};
void *memchr(const void *str, int ch, size_t count) {
    char *ptr = (char *)str;
    while (count) {
        if (*ptr == ch) return (void *)ptr;
        ++ptr;
        --count;
    }
}