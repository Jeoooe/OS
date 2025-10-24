#ifndef OS_ERRNO_H
#define OS_ERRNO_H

//系统出错号变量
extern int errno;

enum FS_ERRNO_TYPE {
    ERROR = 99,     //一般错误
    EPERM = 1,      //操作未许可
    ENOENT,         //文件或目录不存在
    ESRCH,          //进程不存在
    EINTR,          //中断的系统调用
    EIO,            //输出输入错误
    ENXIO,          //指定设备不存在
    E2BIG,          //参数列表太长
    ENOEXEC,        //执行程序格式错误
    EBADF,          //文件句柄错误
    ECHILD,         //子进程不存在
    EAGAIN,         //资源暂时不可用 
    ENOMEM,         //内存不足
    EACCES,         //没有许可权限
    EFAULT,         //地址错误
    ENOTBLK,        //不是块设备
    EBUSY,          //资源忙
    EEXIST,         //文件已存在
    EXDEV,          //非法连接
    ENODEV,         //设备不存在
    ENOTDIR,        //不是目录
    EISDIR,         //是目录
    EINVAL,         //参数无效
    ENFILE,         //系统打开文件太多
    EMFILE,         //打开文件太多
    ENOTTY,         //不恰当IO控制操作
    ETXTBSY,        //不再使用
    EFBIG,          //文件太大
    ENOSPC,         //设备已满
    ESPIPE,         //无效文件指针重定位
    EROFS,          //文件系统只读
    EMLINK,         //连接太多
    EPIPE,          //管道错误
    EDOM,           //域错误
    ERANGE,         //结果过大
    EDEADLK,        //避免资源死锁
    ENAMETOOLONG,   //文件名太长
    ENOLCK,         //没有锁定可以用
    ENOSYS,         //功能未实现
    ENOTEMPTY,      //目录非空
};

#endif