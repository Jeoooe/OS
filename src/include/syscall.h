#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <stdint.h>

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_WRITE,
};

uint32_t getpid();
size_t write(int fd, const char* buf, size_t count);

#endif