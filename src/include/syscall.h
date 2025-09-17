#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <stdint.h>

#define _syscall0(NUMBER) ({        \
    int retval;                     \
    asm volatile(                   \
        "int $0x80\n"                \
        :"=a"(retval)               \
        :"a"(NUMBER)                \
        :"memory"                   \
    );                              \
    retval;                         \
})

#define _syscall1(NUMBER, ARG1) ({        \
    int retval;                     \
    asm volatile(                   \
        "int $0x80\n"                \
        :"=a"(retval)               \
        :"a"(NUMBER),"b"(ARG1)                \
        :"memory"                   \
    );                              \
    retval;                         \
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({        \
    int retval;                     \
    asm volatile(                   \
        "int $0x80\n"                \
        :"=a"(retval)               \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3)                \
        :"memory"                   \
    );                              \
    retval;                         \
})

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
};

uint32_t getpid();
size_t write(int fd, const char* buf, size_t count);

#endif