#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/fs.h>
#include <errno.h>
#include <thread.h>

//打开或创建文件的系统调用
//仅初步实现, 可以打开普通文件
int sys_open(const char* filename, int flag, int mode) {
    inode_t* inode;
    file_t* f;
    int i, fd;
    task_block_t* current;

    mode &= ~current->umask & 0777;

    for (fd = 0; fd < NR_FILE; fd++) {
        if (!current->filp[fd]) break;
    }

    if (fd >= NR_OPEN) return -EINVAL;

    f = file_table;
    for (i = 0;i < NR_FILE;i++, f++) {
        if (!f->count) break;
    }

    if (i >= NR_FILE) return -EINVAL;

    //找到了进程文件表和系统文件表空闲项
    current->filp[fd] = f;
    f->count++;
    //打开
    i = open_namei(filename, flag, mode, &inode);
    if (i < 0) {    //打开失败
        current->filp[fd] = NULL;
        f->count = 0;
        return i;
    }

    //TODO 打开特殊文件, (字符设备, 块设备等)

    //初始化打开文件的结构
    f->mode = inode->mode;
    f->flags = flag;
    f->count = 1;
    f->inode = inode;
    f->pos = 0;
    return fd;
}

//创建文件的系统调用
int sys_creat(const char* pathname, int mode) {
    return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}