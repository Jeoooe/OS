#include <fs/fs.h>
#include <fs/stat.h>
#include <thread.h>
#include <errno.h>

extern int sys_close(unsigned int fd);

//复制文件句柄, fd是复制的文件, arg是新句柄最小值
static int dupfd(unsigned int fd, unsigned int arg) {
    task_block_t *current = running_task();
    if (fd >= NR_OPEN || !current->filp[fd]) {
        return -EBADF;
    }
    if (arg >= NR_OPEN) return -EINVAL;
    while (arg < NR_OPEN) {
        if (current->filp[arg]) arg++;
        else break;
    }
    if (arg >= NR_OPEN) return -EMFILE;
    current->close_on_exec &= ~(1 << arg);
    current->filp[arg] = current->filp[fd];
    current->filp[fd]->count++;
    return arg;
}

int sys_dup2(unsigned int oldfd, unsigned int newfd) {
    sys_close(newfd);
    return dupfd(oldfd, newfd);
}

int sys_dup(unsigned int fd) {
    return dupfd(fd, 0);
}