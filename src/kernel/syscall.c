#include <syscall.h>
#include <debug.h>
#include <thread.h>
#include <stdio.h>
#include <console.h>

#define syscall_nr 32

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

void* syscall_table[syscall_nr];

uint32_t sys_getpid() {
    return running_task()->pid;
}


size_t sys_write(int fd, const char* buf, size_t count) {
    if (fd == stdout || fd == stdin) {
        console_write(buf, count);
        return count;
    }
    return -1;
}


void syscall_init() {
    LOGK("SYSCALL Init...");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
}


uint32_t getpid() {
    return _syscall0(SYS_GETPID);
}

size_t write(int fd, const char* buf, size_t count) {
    return _syscall3(SYS_WRITE, fd, buf, count);
}