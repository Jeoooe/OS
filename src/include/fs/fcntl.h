#ifndef OS_FCNTL_H
#define OS_FCNTL_H

#include <stdint.h>

#define O_ACCMODE 00003         //文件访问模式的屏蔽码

//open()和fcntl()会用到的访问模式
//三选一
#define O_RDONLY    00          //只读
#define O_WRONLY    01          //只写
#define O_RDWR      02          //读写

//文件创建和操作标志
#define O_CREAT     00100       //不存在就创建 仅用于open
#define O_EXCL      00200       //独占使用文件, 用于创建文件时仅允许当前进程创建
#define O_NOCTTY    00400       //不分配控制终端 (?Linus没有实现)
#define O_TRUNC     01000       //写操作，文件截为0
#define O_APPEND    02000       //文件指针指向文件尾部
#define O_NONBLOCK  04000       //非阻塞方式打开和操作文件
#define O_NDELDAY   O_NONBLOCK  //也没有实现

//fcntl的命令
#define F_DUPFD     0   // dup 拷贝文件句柄到最小可用的句柄
#define F_GETFD     1   // get flags 取句柄标志
#define F_SETFD     2   // set flags 设置句柄标志
#define F_GETFL     3
#define F_SETFL     4
//文件锁定命令
#define F_GETLK     5   //获取flock结构
#define F_SETLK     6   //设置(F_RDLOCK or F_WRLCK) 或清除 (F_UNLCK)
#define F_SETLKW    7   //等待设置或者清除锁定

//用于 F_GETFL F_SETFL
#define FD_CLOEXEC  1   //执行时关闭


/*
* 锁定类型
*/
#define F_RDLOCK    0   //读锁定, 即共享锁
#define F_WRLCK     1   //写锁定
#define F_UNLCK     2   //解锁

struct flock {
    int16_t l_type;     //锁定类型
    int16_t l_whence;   //开始偏移
    uint32_t l_start;   //相对偏移
    uint32_t l_len;     //锁定长度
    pid_t l_pid;        //加锁进程的id
};


//以下的函数原型
//创建新文件, 或者重写一个已存在的文件
extern int creat(const char* filename, int mode);

//文件句柄操作
// `fildes` 是文件句柄, `cmd`是操作命令. 有三种形式
// `int fcntl(int fildes, int cmd)`;
// `int fcntl(int fildes, int cmd, long arg)`;
// `int fcntl(int fildes, int cmd, struct flock *lock)`;
extern int fcntl(int fildes, int cmd, ...);

//打开文件
// extern int open(const char *filename, int flags, ...);

#endif