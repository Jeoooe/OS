#include <syscall.h>
#include <debug.h>
#include <thread.h>
#include <stdio.h>
#include <console.h>
#include <memory.h>

#define syscall_nr 32

void* syscall_table[syscall_nr];

extern int sys_fork();  //from thread.c

uint32_t sys_getpid();
size_t sys_write(int fd, const char* buf, size_t count);

void syscall_init() {
    LOGK("SYSCALL Init...");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_FORK] = sys_fork;
}

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



uint32_t getpid() {
    return _syscall0(SYS_GETPID);
}

size_t write(int fd, const char* buf, size_t count) {
    return _syscall3(SYS_WRITE, fd, buf, count);
}

// int fork() {
//     return _syscall0(SYS_FORK);
// }