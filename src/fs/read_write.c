#include <fs/fs.h>
#include <fs/stat.h>
#include <errno.h>
#include <thread.h>
#include <device/dev.h>


extern int file_read(inode_t *inode, file_t *filp, char *buf, int count);
extern int file_write(inode_t *inode, file_t *filp, char *buf, int count);



int sys_read(unsigned int fd, char *buf, int count) {
    file_t *file;
    inode_t *inode;

    //参数合法性
    if (fd >= NR_OPEN || count < 0 || !(file = running_task()->filp[fd])) {
        return -EINVAL;
    }
    if (count == 0) {
        return 0;
    }
    inode = file->inode;

    if (S_ISCHR(inode->mode)) {
        return char_device_request(inode->zones[0], buf, count, 0, 0, REQ_READ);
    }


    //普通的文件
    if (S_ISDIR(inode->mode) || S_ISREG(inode->mode)) {
        if (file->pos + count > inode->size) {
            count = inode->size - file->pos;
        }
        if (count <= 0) return 0;
        return file_read(inode, file, buf, count);
    }

    return -EINVAL;
}

int sys_write(unsigned int fd, char *buf, int count) {
    file_t *file;
    inode_t *inode;

    //参数合法性
    if (fd >= NR_OPEN || count < 0 || !(file = running_task()->filp[fd])) {
        return -EINVAL;
    }
    if (count == 0) {
        return 0;
    }
    inode = file->inode;

    if (S_ISCHR(inode->mode)) {
        return char_device_request(inode->zones[0], buf, count, 0, 0, REQ_WRITE);
    }

    //普通的文件
    if (S_ISDIR(inode->mode) || S_ISREG(inode->mode)) {
        return file_write(inode, file, buf, count);
    }

    return -EINVAL;
}