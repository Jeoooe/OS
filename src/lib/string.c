#include <string.h>
#include <stdint.h>

int strlen(const char* str) {
    int len = 0;
    while (*str++ != EOS) len++;
    return len;
}