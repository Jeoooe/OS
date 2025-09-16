#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <stdint.h>

enum SYSCALL_NR {
    SYS_GETPID = 0,
};

uint32_t getpid();

#endif