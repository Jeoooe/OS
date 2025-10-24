#include <syscall.h>
#include <debug.h>
#include <thread.h>
#include <stdio.h>
#include <console.h>
#include <memory.h>

#define syscall_nr 32

void* syscall_table[syscall_nr];

extern void sys_setup();    //from setup.c
extern int sys_fork();  //from thread.c
extern void sys_exit(int error_code); //from exit.c
extern pid_t sys_waitpid(pid_t pid,int *status, int options);   //from exit.c
extern uint16_t sys_umask(uint16_t mask);   //from system.c
extern int sys_open(const char* filename, int flag, int mode); //from open.c

pid_t sys_getpid();
pid_t sys_getppid();
int sys_write(int fd, const char* buf, size_t count);

void syscall_init() {
    LOGK("SYSCALL Init...");
    syscall_table[SYS_SETUP] = sys_setup;
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_FORK] = sys_fork;
    syscall_table[SYS_GETPPID] = sys_getppid;
    syscall_table[SYS_EXIT] = sys_exit;
    syscall_table[SYS_WAITPID] = sys_waitpid;
    syscall_table[SYS_UMASK] = sys_umask;
    syscall_table[SYS_OPEN] = sys_open;
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



/*
 * 用户态的系统调用函数在下面
 */

void setup() {
    _syscall0(SYS_SETUP);
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
    while (1) ;
}

pid_t waitpid(pid_t pid, int *status, int options) {
    return _syscall3(SYS_WAITPID, pid, status, options);
}

uint16_t umask(uint16_t mask) {
    return _syscall1(SYS_UMASK, (uint32_t)mask);
}

int open(const char* filename, int flag, int mode) {
    return _syscall3(SYS_OPEN, filename, flag, mode);
}