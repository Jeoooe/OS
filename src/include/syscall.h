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
    SYS_SETUP       = 0,
    SYS_EXIT        = 1,
    SYS_FORK        = 2,
    SYS_READ        = 3,
    SYS_WRITE       = 4,
    SYS_OPEN        = 5,
    SYS_CLOSE       = 6,
    SYS_WAITPID     = 7,
    SYS_CREAT       = 8,
    SYS_UNLINK      = 10,
    SYS_MKNOD       = 14,
    SYS_GETPID      = 20,
    SYS_MKDIR       = 39,
    SYS_RMDIR       = 40,
    SYS_DUP         = 41,
    SYS_UMASK       = 60,
    SYS_DUP2        = 63,
    SYS_GETPPID     = 64,
};

/// @brief 获取当前进程pid
/// @return pid
pid_t getpid();

/// @brief 获取子进程pid
/// @return 子进程pid
pid_t getppid();

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

/// @brief 读取数据
/// @param fd 文件描述符
/// @param buf 读取缓冲区
/// @param count 读取字节数
/// @return 读取实际字节数, 失败-1
int read(unsigned int fd, char *buf, int count);

/// @brief 写入数据
/// @param fd 文件描述符
/// @param buf 写入数据的缓冲区
/// @param count 写入数据字节数
/// @return 写入字节数, 失败时返回-1
int write(unsigned int fd, char *buf, int count);

/// @brief 创建特殊文件或节点, 可以添加普通文件, 设备文件或者管道
/// @param filename 文件名
/// @param mode 使用许可和节点类型
/// @param dev 设备号
/// @return 成功0, 否则错误码
int mknod(const char *filename, int mode, int dev);

/// @brief 复制文件句柄
/// @param fd 句柄
/// @return 新句柄
int dup(unsigned int fd);

/// @brief 复制文件句柄到指定句柄
/// @param oldfd 句柄
/// @param newfd 新句柄
/// @return 新句柄
int dup2(unsigned int oldfd, unsigned int newfd);

/// @brief 删除文件链接
/// @param name 文件路径名
/// @return 0或错误码
int unlink(const char *name);

#endif