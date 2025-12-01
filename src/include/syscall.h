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

#define _syscall2(NUMBER, ARG1, ARG2) ({        \
    int retval;                     \
    asm volatile(                   \
        "int $0x80\n"                \
        :"=a"(retval)               \
        :"a"(NUMBER),"b"(ARG1),"c"(ARG2)              \
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
    SYS_SETUP = 0,
    SYS_EXIT = 1,
    SYS_FORK,
    SYS_READ,
    SYS_WRITE,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_WAITPID,
    SYS_GETPID = 20,
    SYS_UMASK = 60,
    SYS_GETPPID = 64,
    SYS_MKDIR = 83,
    SYS_RMDIR = 84,
    SYS_CREAT = 85,
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

/// @brief 修改进程权限
/// @param mask 权限值
/// @return 旧的权限值
uint16_t umask(uint16_t mask);

/// @brief 系统初始化, 只可调用一次
void setup();


int open(const char* filename, int flag, int mode);
int close(uint32_t fp);
int creat(const char* pathname, int mode);

/// @brief 创建目录
/// @param pathname 路径名字
/// @param mode 目录的权限
/// @return 错误码
int mkdir(const char *pathname, int mode);

/// @brief 删除目录
/// @param name 目录名
/// @return 错误码
int rmdir(const char *name);

#endif