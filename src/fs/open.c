#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/fs.h>
#include <errno.h>
#include <thread.h>
#include <assert.h>

//打开或创建文件的系统调用
//仅初步实现, 可以打开普通文件
int sys_open(const char* filename, int flag, int mode) {
    inode_t* inode;
    file_t* f;
    int i, fd;
    task_block_t* current = running_task();

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
    current->close_on_exec &= ~(1 << fd);   //复位close_on_exec
    current->filp[fd] = f;
    f->count++;
    //打开
    i = open_namei(filename, flag, mode, &inode);
    if (i < 0) {    //打开失败
        current->filp[fd] = NULL;
        f->count = 0;
        return i;
    }

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

//关闭文件的系统调用
//成功返回0, 否则返回出错码
int sys_close(uint32_t fd) {
    task_block_t* current = running_task();
    //参数有效性
    if (fd >= NR_OPEN) return -EINVAL;

    file_t* filp = current->filp[fd];
    if (!filp) return -EINVAL;

    current->filp[fd] = NULL;
    if (filp->count == 0) {
        //关闭文件前就已经是0了, 内核就出错了
        panic("Close: file cout is 0");
    }
    
    filp->count--;
    if (filp->count) return 0;
    iput(filp->inode);
    return 0;
}