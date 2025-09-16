#include <syscall.h>
#include <debug.h>
#include <thread.h>

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

void syscall_init() {
    LOGK("SYSCALL Init...");
    syscall_table[SYS_GETPID] = sys_getpid;
}


uint32_t getpid() {
    return _syscall0(SYS_GETPID);
}