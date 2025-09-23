#ifndef OS_FILE_H
#define OS_FILE_H

#include <stdint.h>
#include <fs/inode.h>
#include <ide.h>
#include <fs/dir.h>

typedef struct file_t {
    uint32_t fd_pos;    //文件操作的偏移地址
    uint32_t fd_flag;   //对文件的操作 open_flags, 创建, 读写
    inode_t* fd_inode;
} file_t;

enum bitmap_type {
    INODE_BITMAP,
    BLOCK_BITMAP
};

#define MAX_FILE_OPEN 32

//文件表中获取空闲位
int32_t get_free_slot_in_global();

/* 安装文件描述符下标
 * 返回下标
 */
int32_t pcb_fd_install(int32_t global_fd_index);


//分配一个inode, 返回inode索引
int32_t inode_bitmap_alloc(partition_t* part);

//分配一个块, 返回扇区地址
int32_t block_bitmap_alloc(partition_t* part);

//同步位图512字节到硬盘
void bitmap_sync(partition_t* part, uint32_t bit_index, enum bitmap_type btmp);


/* 创建文件, 返回进程中文件描述符, 否则-1 */
int32_t file_create(dir_t* parent_dir, char* filename, uint8_t flag);

/* 打开文件,返回文件描述符 */
int32_t file_open(uint32_t inode_no, uint8_t flag);

/* 关闭文件 成功0, 失败-1*/
int32_t file_close(file_t* file);


#endif