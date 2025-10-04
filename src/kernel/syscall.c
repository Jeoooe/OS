#include <syscall.h>
#include <debug.h>
#include <thread.h>
#include <stdio.h>
#include <console.h>
#include <memory.h>

#define syscall_nr 32

void* syscall_table[syscall_nr];

extern int sys_fork();  //from thread.c
extern void sys_exit(int error_code); //from exit.c
extern pid_t sys_waitpid(pid_t pid,int *status, int options);   //from exit.c

pid_t sys_getpid();
pid_t sys_getppid();
int sys_write(int fd, const char* buf, size_t count);

void syscall_init() {
    LOGK("SYSCALL Init...");
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_FORK] = sys_fork;
    syscall_table[SYS_GETPPID] = sys_getppid;
    syscall_table[SYS_EXIT] = sys_exit;
    syscall_table[SYS_WAITPID] = sys_waitpid;
}

pid_t sys_getpid() {
    return running_task()->pid;
}
pid_t sys_getppid() {
    return running_task()->ppid;
}


int sys_write(int fd, const char* buf, size_t count) {
    if (fd == stdout || fd == stdin) {
        console_write(buf, count);
        return count;
    }
    return -1;
}



pid_t getpid() {
    return _syscall0(SYS_GETPID);
}

pid_t getppid() {
    return _syscall0(SYS_GETPPID);
}

int write(int fd, const char* buf, size_t count) {
    return _syscall3(SYS_WRITE, fd, buf, count);
}

[[noreturn]] void exit(int error_code) {
    _syscall1(SYS_EXIT, error_code);
}

pid_t waitpid(pid_t pid, int *status, int options) {
    return _syscall3(SYS_WAITPID, pid, status, options);
}

