#ifndef OS_FILE_H
#define OS_FILE_H

#include <stdint.h>
#include <fs/inode.h>

typedef struct file_t {
    uint32_t fd_pos;    //文件操作的偏移地址
    uint32_t fd_flag;
    inode_t* fd_inode;
} file_t;

enum bitmap_type {
    INODE_BITMAP,
    BLOCK_BITMAP
};

#define MAX_FILE_OPEN 32


#endif