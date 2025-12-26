/*
 *  进程信号控制部分
*/


#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdint.h>

#define SIGHUP    1   // 终端挂断
#define SIGINT    2   // 中断（Ctrl+C）
#define SIGQUIT   3   // 退出（Ctrl+\）
#define SIGILL    4   // 非法指令
#define SIGABRT   6   // 异常终止
#define SIGFPE    8   // 浮点异常
#define SIGKILL   9   // 强制杀死（不可捕获）
#define SIGSEGV  11   // 段错误
#define SIGPIPE  13   // 管道破裂
#define SIGALRM  14   // 定时器超时
#define SIGTERM  15   // 终止（可捕获）
#define SIGCHLD  17   // 子进程状态改变
#define SIGCONT  18   // 继续执行
#define SIGSTOP  19   // 停止（不可捕获）
#define SIGTSTP  20   // 终端停止（Ctrl+Z）

#define SIG_DFL ((void (*)(int))0)      //默认处理句柄
#define SIG_IGN ((void (*)(int))1)      //忽略处理句柄

//信号处理函数原型
typedef void (*signal_handler_t)(int);

typedef struct signal_table_t {
    signal_handler_t sa_handler[32];    //信号处理函数
    uint32_t pending;   //挂起的信号位图
    uint32_t blocked;   //屏蔽信号位图
} signal_table_t;

//signal系统调用, 指定信号处理函数
signal_handler_t sys_signal(int signum, signal_handler_t handler);

#endif