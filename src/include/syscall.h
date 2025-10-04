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
    SYS_GETPPID,
    SYS_WRITE,
    SYS_FORK = 57,
    SYS_EXIT,
    SYS_WAITPID,
};

/// @brief 获取当前进程pid
/// @return pid
pid_t getpid();

/// @brief 获取子进程pid
/// @return 子进程pid
pid_t getppid();

/// @brief 写入数据
/// @param fd 文件描述符
/// @param buf 写入数据的缓冲区
/// @param count 写入数据字节数
/// @return 写入字节数, 失败时返回-1
int write(int fd, const char* buf, size_t count);

/// @brief 创建子进程
/// @return 父进程返回子进程pid, 子进程返回0
extern int fork();

/// @brief 退出进程
/// @param error_code 状态码 
void exit(int error_code);

/// @brief 等待子进程退出
/// @param pid 等待子进程的pid, 0为监听所有子进程
/// @param status 保存进程的状态
/// @param options 可选项 暂时没有实现
/// @return 等待进程的pid
pid_t waitpid(pid_t pid, int *status, int options);

#endif