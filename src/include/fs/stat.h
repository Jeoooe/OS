#ifndef OS_STAT_H
#define OS_STAT_H

//文件类型
#define S_IFMT 00170000         //类型屏蔽码
#define S_IFREG 0100000         //常规文件
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG) //是否是常规文件
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR) //是否是目录文件
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR) //是否是字符设备文件
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK) //是否是块设备文件
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFFIFO) //是否是FIFO文件

//文件属性位

#define S_ISUID 0004000     //执行时设置用户ID
#define S_ISGID 0002000     //执行时设置组ID
#define S_ISVTX 0001000     //目录 受限删除

//访问权限

//宿主权限
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

//宿主用户组权限
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

//其他人权限
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001


#endif